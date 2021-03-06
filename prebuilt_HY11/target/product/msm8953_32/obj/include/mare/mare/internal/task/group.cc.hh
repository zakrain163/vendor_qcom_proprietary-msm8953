// --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
// Copyright 2013-2015 Qualcomm Technologies, Inc.
// All rights reserved.
// Confidential and Proprietary – Qualcomm Technologies, Inc.
// --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
#pragma once

#include <mare/group.hh>

#include <mare/internal/task/group.hh>
#include <mare/internal/task/task.hh>
#include <mare/internal/task/taskfactory.hh>

namespace mare {
namespace internal {

inline
::mare::internal::group * c_ptr(::mare::group* g) {
  return g->get_raw_ptr();
}

namespace group_launch {

template<typename Code, typename... Args>
struct launch_code {

  using decayed_type = typename std::decay<Code>::type;

  static_assert(::mare::internal::is_mare_task20_ptr<decayed_type>::value == false,
                "Invalid launch for Code");

  template<typename CodeType = Code, typename T>
  static void launch_impl(bool notify, ::mare::internal::group* gptr, CodeType&& code, T&&, Args&&... args){

    using factory = ::mare::internal::task_factory<CodeType>;

    factory::launch(notify, gptr, std::forward<CodeType>(code), std::forward<Args>(args)...);
  }

};

template<typename T, typename...Args>
struct launch_task {

  using decayed_type = typename std::decay<T>::type;

  static_assert(::mare::internal::is_mare_task20_ptr<decayed_type>::value,
                "Invalid launch for tasks");

  static void launch_impl(bool notify,
                            ::mare::internal::group* gptr,
                            T&& t,
                            std::true_type,
                            Args&&... args) {
    MARE_UNUSED(notify);

    static_assert(is_mare_task20_ptr<decayed_type>::has_arguments == true,
                    "Invalid number of arguments for launching a task by the group.");

    auto ptr = ::mare::internal::get_cptr(t);

    MARE_INTERNAL_ASSERT(ptr != nullptr, "Unexpected null task<>.");

    t->bind_all(std::forward<Args>(args)...);
    ptr->launch(gptr, nullptr);
  }

  static void launch_impl(bool notify,
                            ::mare::internal::group* gptr,
                            T&& t,
                            std::false_type,
                            Args&&... ) {
    MARE_UNUSED(notify);

    static_assert(sizeof...(Args) == 0,
                  "Invalid number of arguments for launching a task by the group.");

    task* ptr = ::mare::internal::get_cptr(t);
    MARE_INTERNAL_ASSERT(ptr != nullptr, "Unexpected null task<>.");

    ptr->launch(gptr, nullptr);
  }
};

};
};

inline void
group::add(task_ptr<> const& task)
{
  MARE_INTERNAL_ASSERT(get_raw_ptr() != nullptr, "Unexpected null group.");
  auto t = internal::c_ptr(task);
  MARE_API_ASSERT(t, "null task pointer.");
  t->join_group(get_raw_ptr(), true);
}

template<typename TaskPtrOrCode, typename...Args>
void
group::launch(TaskPtrOrCode&& task_or_code, Args&& ...args)
{
  auto g = get_raw_ptr();
  MARE_INTERNAL_ASSERT(g != nullptr, "Unexpected null group.");

  using decayed_type = typename std::decay<TaskPtrOrCode>::type;

  using launch_policy = typename std::conditional<
    internal::is_mare_task20_ptr<decayed_type>::value,
    internal::group_launch::launch_task<TaskPtrOrCode, Args...> ,
    internal::group_launch::launch_code<TaskPtrOrCode, Args...> >::type;

  using has_args = typename std::conditional<
    sizeof...(Args) == 0,
    std::false_type,
    std::true_type>::type;

  launch_policy::launch_impl(true,
                             g,
                             std::forward<TaskPtrOrCode>(task_or_code),
                             has_args(),
                             std::forward<Args>(args)...);
}

};
