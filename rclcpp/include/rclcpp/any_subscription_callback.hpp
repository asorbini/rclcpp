// Copyright 2014 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef RCLCPP_RCLCPP_ANY_SUBSCRIPTION_CALLBACK_HPP_
#define RCLCPP_RCLCPP_ANY_SUBSCRIPTION_CALLBACK_HPP_

#include <rclcpp/allocator/allocator_common.hpp>
#include <rclcpp/function_traits.hpp>

#include <functional>
#include <memory>
#include <type_traits>

#include <rmw/types.h>

namespace rclcpp
{

namespace any_subscription_callback
{

template<typename MessageT, typename Alloc>
class AnySubscriptionCallback
{
  using MessageAlloc = allocator::AllocRebind<MessageT, Alloc>;
  using MessageDeleter = allocator::Deleter<typename MessageAlloc::allocator_type, MessageT>;
  using UniquePtr = std::unique_ptr<MessageT, MessageDeleter>;

  using SharedPtrCallback = std::function<void(const std::shared_ptr<MessageT>)>;
  using SharedPtrWithInfoCallback =
      std::function<void(const std::shared_ptr<MessageT>, const rmw_message_info_t &)>;
  using ConstSharedPtrCallback = std::function<void(const std::shared_ptr<const MessageT>)>;
  using ConstSharedPtrWithInfoCallback =
      std::function<void(const std::shared_ptr<const MessageT>, const rmw_message_info_t &)>;
  using UniquePtrCallback = std::function<void(UniquePtr)>;
  using UniquePtrWithInfoCallback = std::function<void(UniquePtr, const rmw_message_info_t &)>;

  SharedPtrCallback shared_ptr_callback_;
  SharedPtrWithInfoCallback shared_ptr_with_info_callback_;
  ConstSharedPtrCallback const_shared_ptr_callback_;
  ConstSharedPtrWithInfoCallback const_shared_ptr_with_info_callback_;
  UniquePtrCallback unique_ptr_callback_;
  UniquePtrWithInfoCallback unique_ptr_with_info_callback_;

public:
  AnySubscriptionCallback(std::shared_ptr<Alloc> allocator)
  : shared_ptr_callback_(nullptr), shared_ptr_with_info_callback_(nullptr),
    const_shared_ptr_callback_(nullptr), const_shared_ptr_with_info_callback_(nullptr),
    unique_ptr_callback_(nullptr), unique_ptr_with_info_callback_(nullptr)
  {
    message_allocator_ = new typename MessageAlloc::allocator_type(*allocator.get());
    allocator::set_allocator_for_deleter(&message_deleter_, message_allocator_);
  }

  AnySubscriptionCallback(const AnySubscriptionCallback &) = default;

  template<
    typename CallbackT,
    typename std::enable_if<
      rclcpp::same_arguments<
        CallbackT,
        SharedPtrCallback
      >::value
    >::type * = nullptr
  >
  void set(CallbackT callback)
  {
    shared_ptr_callback_ = callback;
  }

  template<
    typename CallbackT,
    typename std::enable_if<
      rclcpp::same_arguments<
        CallbackT,
        SharedPtrWithInfoCallback
      >::value
    >::type * = nullptr
  >
  void set(CallbackT callback)
  {
    shared_ptr_with_info_callback_ = callback;
  }

  template<
    typename CallbackT,
    typename std::enable_if<
      rclcpp::same_arguments<
        CallbackT,
        ConstSharedPtrCallback
      >::value
    >::type * = nullptr
  >
  void set(CallbackT callback)
  {
    const_shared_ptr_callback_ = callback;
  }

  template<
    typename CallbackT,
    typename std::enable_if<
      rclcpp::same_arguments<
        CallbackT,
        ConstSharedPtrWithInfoCallback
      >::value
    >::type * = nullptr
  >
  void set(CallbackT callback)
  {
    const_shared_ptr_with_info_callback_ = callback;
  }

  template<
    typename CallbackT,
    typename std::enable_if<
      rclcpp::same_arguments<
        CallbackT,
        UniquePtrCallback
      >::value
    >::type * = nullptr
  >
  void set(CallbackT callback)
  {
    unique_ptr_callback_ = callback;
  }

  template<
    typename CallbackT,
    typename std::enable_if<
      rclcpp::same_arguments<
        CallbackT,
        UniquePtrWithInfoCallback
      >::value
    >::type * = nullptr
  >
  void set(CallbackT callback)
  {
    unique_ptr_with_info_callback_ = callback;
  }

  void dispatch(
    std::shared_ptr<MessageT> message, const rmw_message_info_t & message_info)
  {
    (void)message_info;
    if (shared_ptr_callback_) {
      shared_ptr_callback_(message);
    } else if (shared_ptr_with_info_callback_) {
      shared_ptr_with_info_callback_(message, message_info);
    } else if (const_shared_ptr_callback_) {
      const_shared_ptr_callback_(message);
    } else if (const_shared_ptr_with_info_callback_) {
      const_shared_ptr_with_info_callback_(message, message_info);
    } else if (unique_ptr_callback_) {
      auto ptr = MessageAlloc::allocate(*message_allocator_, 1);
      MessageAlloc::construct(*message_allocator_, ptr, *message);
      unique_ptr_callback_(UniquePtr(ptr, message_deleter_));
    } else if (unique_ptr_with_info_callback_) {
      auto ptr = MessageAlloc::allocate(*message_allocator_, 1);
      MessageAlloc::construct(*message_allocator_, ptr, *message);
      unique_ptr_with_info_callback_(UniquePtr(ptr, message_deleter_), message_info);
    } else {
      throw std::runtime_error("unexpected message without any callback set");
    }
  }

  void dispatch_intra_process(
    UniquePtr & message, const rmw_message_info_t & message_info)
  {
    (void)message_info;
    if (shared_ptr_callback_) {
      typename MessageT::SharedPtr shared_message = std::move(message);
      shared_ptr_callback_(shared_message);
    } else if (shared_ptr_with_info_callback_) {
      typename MessageT::SharedPtr shared_message = std::move(message);
      shared_ptr_with_info_callback_(shared_message, message_info);
    } else if (const_shared_ptr_callback_) {
      typename MessageT::ConstSharedPtr const_shared_message = std::move(message);
      const_shared_ptr_callback_(const_shared_message);
    } else if (const_shared_ptr_with_info_callback_) {
      typename MessageT::ConstSharedPtr const_shared_message = std::move(message);
      const_shared_ptr_with_info_callback_(const_shared_message, message_info);
    } else if (unique_ptr_callback_) {
      unique_ptr_callback_(std::move(message));
    } else if (unique_ptr_with_info_callback_) {
      unique_ptr_with_info_callback_(std::move(message), message_info);
    } else {
      throw std::runtime_error("unexpected message without any callback set");
    }
  }

private:
  typename MessageAlloc::allocator_type * message_allocator_;
  MessageDeleter message_deleter_;
};

} /* namespace any_subscription_callback */
} /* namespace rclcpp */

#endif /* RCLCPP_RCLCPP_ANY_SUBSCRIPTION_CALLBACK_HPP_ */
