//  Copyright (c) 2007-2012 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_RUNTIME_ACTIONS_COMPONENT_ACTION_MAR_26_2008_1054AM)
#define HPX_RUNTIME_ACTIONS_COMPONENT_ACTION_MAR_26_2008_1054AM

#include <cstdlib>
#include <stdexcept>

#include <hpx/hpx_fwd.hpp>
#include <hpx/config.hpp>
#include <hpx/config/bind.hpp>
#include <hpx/exception.hpp>
#include <hpx/runtime/naming/address.hpp>
#include <hpx/runtime/actions/continuation.hpp>
#include <hpx/runtime/actions/action_support.hpp>
#include <hpx/runtime/components/console_error_sink.hpp>
#include <hpx/util/unused.hpp>
#include <hpx/util/void_cast.hpp>

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repeat.hpp>
#if BOOST_WORKAROUND(BOOST_MSVC, == 1600)
#include <boost/mpl/identity.hpp>
#endif

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace actions
{
    ///////////////////////////////////////////////////////////////////////////
#define HPX_FUNCTION_ARG_ENUM(z, n, data)                                     \
        BOOST_PP_CAT(component_action_arg, BOOST_PP_INC(n)) =                 \
            component_action_base + BOOST_PP_INC(n),                          \
    /**/
#define HPX_FUNCTION_RETARG_ENUM(z, n, data)                                  \
        BOOST_PP_CAT(component_result_action_arg, BOOST_PP_INC(n)) =          \
            component_result_action_base + BOOST_PP_INC(n),                   \
    /**/

    enum component_action
    {
        /// remotely callable member function identifiers
        component_action_base = 1000,
        component_action_arg0 = component_action_base + 0,
        BOOST_PP_REPEAT(HPX_ACTION_ARGUMENT_LIMIT, HPX_FUNCTION_ARG_ENUM, _)

        /// remotely callable member function identifiers with result
        component_result_action_base = 2000,
        BOOST_PP_REPEAT(HPX_ACTION_ARGUMENT_LIMIT, HPX_FUNCTION_RETARG_ENUM, _)
        component_result_action_arg0 = component_result_action_base + 0
    };

#undef HPX_FUNCTION_RETARG_ENUM
#undef HPX_FUNCTION_ARG_ENUM

    ///////////////////////////////////////////////////////////////////////////
    //  Specialized generic component action types allowing to hold a different
    //  number of arguments
    ///////////////////////////////////////////////////////////////////////////

    // zero argument version
    template <
        typename Component, typename Result, int Action,
        Result (Component::*F)(), typename Derived,
        threads::thread_priority Priority = threads::thread_priority_default>
    class base_result_action0
      : public action<Component, Action, Result, hpx::util::tuple0<>,
                      Derived, Priority>
    {
    public:
        typedef Result result_type;
        typedef hpx::util::tuple0<> arguments_type;
        typedef action<Component, Action, result_type, arguments_type,
                       Derived, Priority>
            base_type;

    protected:
        /// The \a continuation_thread_function will be registered as the thread
        /// function of a thread. It encapsulates the execution of the
        /// original function (given by \a func), while ignoring the return
        /// value.
        template <typename Address>   // dummy template parameter
        static threads::thread_state_enum
        thread_function(Address lva)
        {
            try {
                LTM_(debug) << "Executing component action("
                            << detail::get_action_name<Derived>()
                            << ") lva(" << reinterpret_cast<void const*>
                                (get_lva<Component>::call(lva)) << ")";
                (get_lva<Component>::call(lva)->*F)();      // just call the function
            }
            catch (hpx::exception const& e) {
                if (e.get_error() != hpx::thread_interrupted) {
                    LTM_(error)
                        << "Unhandled exception while executing component action("
                        << detail::get_action_name<Derived>()
                        << ") lva(" << reinterpret_cast<void const*>
                            (get_lva<Component>::call(lva)) << "): " << e.what();

                    // report this error to the console in any case
                    hpx::report_error(boost::current_exception());
                }
            }
            return threads::terminated;
        }

    public:
        /// \brief This static \a construct_thread_function allows to construct
        /// a proper thread function for a \a thread without having to
        /// instantiate the \a base_result_action0 type. This is used by the \a
        /// applier in case no continuation has been supplied.
        template <typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_thread_function(naming::address::address_type lva,
            BOOST_FWD_REF(Arguments) /*args*/)
        {
            threads::thread_state_enum (*f)(naming::address::address_type) =
                &Derived::template thread_function<naming::address::address_type>;

            return HPX_STD_BIND(f, lva);
        }

        /// \brief This static \a construct_thread_function allows to construct
        /// a proper thread function for a \a thread without having to
        /// instantiate the \a base_result_action0 type. This is used by the \a
        /// applier in case a continuation has been supplied
        template <typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_thread_function(continuation_type& cont,
            naming::address::address_type lva, BOOST_FWD_REF(Arguments) args)
        {
            return boost::move(
                base_type::construct_continuation_thread_object_function(
                    cont, F, get_lva<Component>::call(lva),
                    boost::forward<Arguments>(args)));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <
        typename Component, typename Result, int Action,
        Result (Component::*F)(),
        threads::thread_priority Priority = threads::thread_priority_default,
        typename Derived = detail::this_type>
    struct result_action0
      : base_result_action0<Component, Result, Action, F,
            typename detail::action_type<
                result_action0<Component, Result, Action, F, Priority>,
                Derived
            >::type, Priority>
    {
        typedef typename detail::action_type<
            result_action0<Component, Result, Action, F, Priority>, Derived
        >::type derived_type;

        typedef boost::mpl::false_ direct_execution;
    };

#if BOOST_WORKAROUND(BOOST_MSVC, <= 1700)
#if BOOST_WORKAROUND(BOOST_MSVC, == 1600)
    namespace detail
    {
        template <typename Obj, typename Result>
        struct synthesize_const_mf<Obj, Result (*)()>
        {
            typedef Result (Obj::*type)() const;
        };

        template <typename Obj, typename Result>
        struct synthesize_const_mf<Obj, Result (Obj::*)() const>
        {
            typedef Result (Obj::*type)() const;
        };

        template <typename Result>
        typename boost::mpl::identity<Result (*)()>::type
        replicate_type(Result (*p)());
    }
#endif

    template <typename Component, typename Result,
        Result (Component::*F)()>
    struct make_action<Result (Component::*)(), F, boost::mpl::false_>
      : result_action0<Component, Result, component_result_action_arg0, F>
    {};

    template <typename Component, typename Result,
        Result (Component::*F)() const>
    struct make_action<Result (Component::*)() const, F, boost::mpl::false_>
      : result_action0<Component const, Result, component_result_action_arg0, F>
    {};

#else

    template <typename Component, typename Result,
        Result (Component::*F)()>
    struct make_action<Result (Component::*)(), F, boost::mpl::false_>
      : boost::mpl::identity<result_action0<
            Component, Result, component_result_action_arg0, F> >
    {};

    template <typename Component, typename Result,
        Result (Component::*F)() const>
    struct make_action<Result (Component::*)() const, F, boost::mpl::false_>
      : boost::mpl::identity<result_action0<
            Component const, Result, component_result_action_arg0, F> >
    {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    template <
        typename Component, typename Result, int Action,
        Result (Component::*F)(), typename Derived = detail::this_type>
    struct direct_result_action0
      : public base_result_action0<Component, Result, Action, F,
            typename detail::action_type<
                direct_result_action0<Component, Result, Action, F>, Derived
            >::type>
    {
        typedef typename detail::action_type<
            direct_result_action0<Component, Result, Action, F>, Derived
        >::type derived_type;

        typedef boost::mpl::true_ direct_execution;

        template <typename Arguments>
        static Result
        execute_function(naming::address::address_type lva,
            BOOST_FWD_REF(Arguments))
        {
            LTM_(debug)
                << "direct_result_action0::execute_function: name("
                << detail::get_action_name<derived_type>()
                << ") lva(" << reinterpret_cast<void const*>(
                    get_lva<Component>::call(lva)) << ")";

            return (get_lva<Component>::call(lva)->*F)();
        }

        /// The function \a get_action_type returns whether this action needs
        /// to be executed in a new thread or directly.
        static base_action::action_type get_action_type()
        {
            return base_action::direct_action;
        }
    };

#if BOOST_WORKAROUND(BOOST_MSVC, <= 1700)
    template <typename Component, typename Result,
        Result (Component::*F)()>
    struct make_action<Result (Component::*)(), F, boost::mpl::true_>
      : direct_result_action0<Component, Result,
            component_result_action_arg0, F>
    {};

    template <typename Component, typename Result,
        Result (Component::*F)() const>
    struct make_action<Result (Component::*)() const, F, boost::mpl::true_>
      : direct_result_action0<Component const, Result,
            component_result_action_arg0, F>
    {};
#else
    template <typename Component, typename Result,
        Result (Component::*F)()>
    struct make_action<Result (Component::*)(), F, boost::mpl::true_>
      : boost::mpl::identity<direct_result_action0<Component, Result,
            component_result_action_arg0, F> >
    {};

    template <typename Component, typename Result,
        Result (Component::*F)() const>
    struct make_action<Result (Component::*)() const, F, boost::mpl::true_>
      : direct_result_action0<Component const, Result,
            component_result_action_arg0, F>
    {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    //  zero parameter version, no result value
    template <typename Component, int Action, void (Component::*F)(), typename Derived,
      threads::thread_priority Priority = threads::thread_priority_default>
    class base_action0
      : public action<Component, Action, util::unused_type,
                      hpx::util::tuple0<>, Derived, Priority>
    {
    public:
        typedef util::unused_type result_type;
        typedef hpx::util::tuple0<> arguments_type;
        typedef action<Component, Action, result_type, arguments_type,
                       Derived, Priority>
            base_type;

    protected:
        /// The \a continuation_thread_function will be registered as the thread
        /// function of a thread. It encapsulates the execution of the
        /// original function (given by \a func), while ignoring the return
        /// value.
        template <typename Address>   // dummy template parameter
        static threads::thread_state_enum
        thread_function(Address lva)
        {
            try {
                LTM_(debug) << "Executing component action("
                            << detail::get_action_name<Derived>()
                            << ") lva(" << reinterpret_cast<void const*>
                                (get_lva<Component>::call(lva)) << ")";
                (get_lva<Component>::call(lva)->*F)();      // just call the function
            }
            catch (hpx::exception const& e) {
                if (e.get_error() != hpx::thread_interrupted) {
                    LTM_(error)
                        << "Unhandled exception while executing component action("
                        << detail::get_action_name<Derived>()
                        << ") lva(" << reinterpret_cast<void const*>
                            (get_lva<Component>::call(lva)) << "): " << e.what();

                    // report this error to the console in any case
                    hpx::report_error(boost::current_exception());
                }
            }
            return threads::terminated;
        }

    public:
        /// \brief This static \a construct_thread_function allows to construct
        /// a proper thread function for a \a thread without having to
        /// instantiate the base_action0 type. This is used by the \a applier in
        /// case no continuation has been supplied.
        template <typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_thread_function(naming::address::address_type lva,
            BOOST_FWD_REF(Arguments) /*args*/)
        {
            threads::thread_state_enum (*f)(naming::address::address_type) =
                &Derived::template thread_function<naming::address::address_type>;

            return HPX_STD_BIND(f, lva);
        }

        /// \brief This static \a construct_thread_function allows to construct
        /// a proper thread function for a \a thread without having to
        /// instantiate the base_action0 type. This is used by the \a applier in
        /// case a continuation has been supplied
        template <typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_thread_function(continuation_type& cont,
            naming::address::address_type lva, BOOST_FWD_REF(Arguments) args)
        {
            return boost::move(
                base_type::construct_continuation_thread_object_function_void(
                    cont, F, get_lva<Component>::call(lva),
                    boost::forward<Arguments>(args)));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Component, int Action, void (Component::*F)(),
        threads::thread_priority Priority = threads::thread_priority_default,
        typename Derived = detail::this_type>
    struct action0
      : base_action0<Component, Action, F,
            typename detail::action_type<
                action0<Component, Action, F, Priority>, Derived
            >::type, Priority>
    {
        typedef typename detail::action_type<
            action0<Component, Action, F, Priority>, Derived
        >::type derived_type;

        typedef boost::mpl::false_ direct_execution;
    };

#if BOOST_WORKAROUND(BOOST_MSVC, <= 1700)
    template <typename Component, void (Component::*F)()>
    struct make_action<void (Component::*)(), F, boost::mpl::false_>
      : action0<Component, component_result_action_arg0, F>
    {};

    template <typename Component, void (Component::*F)() const>
    struct make_action<void (Component::*)() const, F, boost::mpl::false_>
      : action0<Component const, component_result_action_arg0, F>
    {};
#else
    template <typename Component, void (Component::*F)()>
    struct make_action<void (Component::*)(), F, boost::mpl::false_>
      : boost::mpl::identity<action0<
            Component, component_result_action_arg0, F> >
    {};

    template <typename Component, void (Component::*F)() const>
    struct make_action<void (Component::*)() const, F, boost::mpl::false_>
      : boost::mpl::identity<action0<
            Component const, component_result_action_arg0, F> >
    {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    template <typename Component, int Action, void (Component::*F)(),
        typename Derived = detail::this_type>
    struct direct_action0
      : base_action0<Component, Action, F,
            typename detail::action_type<
                direct_action0<Component, Action, F>, Derived
            >::type>
    {
        typedef typename detail::action_type<
            direct_action0<Component, Action, F>, Derived
        >::type derived_type;

        typedef boost::mpl::true_ direct_execution;

        template <typename Arguments>
        static util::unused_type
        execute_function(naming::address::address_type lva,
            BOOST_FWD_REF(Arguments))
        {
            LTM_(debug)
                << "direct_action0::execute_function: name("
                << detail::get_action_name<derived_type>()
                << ") lva(" << reinterpret_cast<void const*>(
                    get_lva<Component>::call(lva)) << ")";
            (get_lva<Component>::call(lva)->*F)();
            return util::unused;
        }

        /// The function \a get_action_type returns whether this action needs
        /// to be executed in a new thread or directly.
        static base_action::action_type get_action_type()
        {
            return base_action::direct_action;
        }
    };

#if BOOST_WORKAROUND(BOOST_MSVC, <= 1700)
    template <typename Component, void (Component::*F)()>
    struct make_action<void (Component::*)(), F, boost::mpl::true_>
      : direct_action0<Component, component_result_action_arg0, F>
    {};

    template <typename Component, void (Component::*F)() const>
    struct make_action<void (Component::*)() const, F, boost::mpl::true_>
      : direct_action0<Component const, component_result_action_arg0, F>
    {};
#else
    template <typename Component, void (Component::*F)()>
    struct make_action<void (Component::*)(), F, boost::mpl::true_>
      : boost::mpl::identity<direct_action0<
            Component, component_result_action_arg0, F> >
    {};

    template <typename Component, void (Component::*F)() const>
    struct make_action<void (Component::*)() const, F, boost::mpl::true_>
      : boost::mpl::identity<direct_action0<
            Component const, component_result_action_arg0, F> >
    {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    // the specialization for void return type is just a template alias
    template <
        typename Component, int Action,
        void (Component::*F)(),
        threads::thread_priority Priority,
        typename Derived>
    struct result_action0<Component, void, Action, F, Priority, Derived>
        : action0<Component, Action, F, Priority, Derived>
    {};
}}

///////////////////////////////////////////////////////////////////////////////
#define HPX_DEFINE_COMPONENT_ACTION(component, func, name)                    \
    typedef HPX_MAKE_COMPONENT_ACTION(component, func) name                   \
    /**/
#define HPX_DEFINE_COMPONENT_CONST_ACTION(component, func, name)              \
    typedef HPX_MAKE_CONST_COMPONENT_ACTION(component, func) name             \
    /**/
#define HPX_DEFINE_COMPONENT_DIRECT_ACTION(component, func, name)             \
    typedef HPX_MAKE_DIRECT_COMPONENT_ACTION(component, func) name            \
    /**/
#define HPX_DEFINE_COMPONENT_CONST_DIRECT_ACTION(component, func, name)       \
    typedef HPX_MAKE_CONST_DIRECT_COMPONENT_ACTION(component, func) name      \
    /**/

#define HPX_DEFINE_COMPONENT_ACTION_TPL(component, func, name)                \
    typedef HPX_MAKE_COMPONENT_ACTION_TPL(component, func) name               \
    /**/
#define HPX_DEFINE_COMPONENT_CONST_ACTION_TPL(component, func, name)          \
    typedef HPX_MAKE_CONST_COMPONENT_ACTION_TPL(component, func) name         \
    /**/
#define HPX_DEFINE_COMPONENT_DIRECT_ACTION_TPL(component, func, name)         \
    typedef HPX_MAKE_DIRECT_COMPONENT_ACTION_TPL(component, func) name        \
    /**/
#define HPX_DEFINE_COMPONENT_CONST_DIRECT_ACTION_TPL(component, func, name)   \
    typedef HPX_MAKE_CONST_DIRECT_COMPONENT_ACTION_TPL(component, func) name  \
    /**/

///////////////////////////////////////////////////////////////////////////////
// bring in the rest of the implementations
#include <hpx/runtime/actions/component_action_implementations.hpp>

///////////////////////////////////////////////////////////////////////////////
// Register the action templates with serialization.
HPX_SERIALIZATION_REGISTER_TEMPLATE(
    (template <typename Action>), (hpx::actions::transfer_action<Action>)
)

#include <hpx/config/warnings_suffix.hpp>

#endif

