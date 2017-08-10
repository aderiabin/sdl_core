/*
 * Copyright (c) 2017, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE. */

#include "gtest/gtest.h"
#include "remote_control/resource_allocation_manager_impl.h"
#include "mock_resource_allocation_manager.h"
#include "mock_remote_control_plugin.h"
#include "remote_control/rc_app_extension.h"
#include "remote_control/rc_module_constants.h"
#include "remote_control/message_helper.h"
#include "functional_module/module_observer.h"
#include "application_manager/mock_application.h"
#include "include/mock_service.h"
#include "utils/shared_ptr.h"
#include "utils/make_shared.h"
#include "interfaces/HMI_API.h"

using functional_modules::PluginInfo;
using functional_modules::ProcessResult;
using functional_modules::RCFunctionID;
using application_manager::ServicePtr;

using application_manager::MockService;
using namespace test::components;
using application_manager_test::MockApplication;

using ::testing::_;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::Eq;
using ::application_manager::Message;
using ::application_manager::MessageType;
using ::application_manager::ApplicationSharedPtr;
using ::protocol_handler::MessagePriority;

namespace {
const bool kDeviceHandle = 1u;
const std::string kModuleType1 = "CLIMATE";
const std::string kModuleType2 = "RADIO";
const int32_t kConnectionKey = 5;
const int32_t kCorrelationId = 5;
const int kModuleId = 153;
const uint32_t kAppId1 = 11u;
const uint32_t kAppId2 = 22u;
}

namespace remote_control {

class RAManagerTest : public ::testing::Test {
 public:
  RAManagerTest()
      : mock_service_(utils::MakeShared<NiceMock<MockService> >())
      , mock_app_1_(utils::MakeShared<NiceMock<MockApplication> >())
      , mock_app_2_(utils::MakeShared<NiceMock<MockApplication> >())
      , message_(utils::MakeShared<Message>(MessagePriority::FromServiceType(
            protocol_handler::ServiceType::kRpc))) {
    EXPECT_CALL(mock_module_, service()).WillRepeatedly(Return(mock_service_));
  }

  void CheckResultWithHMILevelAndAccessMode(
      ResourceAllocationManagerImpl& ra_manager,
      mobile_apis::HMILevel::eType app_level,
      const remote_control::AcquireResult::eType expected_result,
      const hmi_apis::Common_RCAccessMode::eType access_mode) {
    // Arrange
    EXPECT_CALL(*mock_service_, GetApplication(kAppId1))
        .WillOnce(Return(mock_app_1_));
    ra_manager.SetAccessMode(access_mode);
    EXPECT_EQ(remote_control::AcquireResult::ALLOWED,
              ra_manager.AcquireResource(kModuleType1, kAppId1));
    EXPECT_CALL(*mock_service_, GetApplication(kAppId2))
        .WillOnce(Return(mock_app_2_));
    EXPECT_CALL(*mock_app_2_, hmi_level()).WillOnce(Return(app_level));
    // Second app tries to get already acquired resource by 1st app
    EXPECT_EQ(expected_result,
              ra_manager.AcquireResource(kModuleType1, kAppId2));
  }

 protected:
  utils::SharedPtr<NiceMock<MockService> > mock_service_;
  utils::SharedPtr<NiceMock<MockApplication> > mock_app_1_;
  utils::SharedPtr<NiceMock<MockApplication> > mock_app_2_;
  application_manager::MessagePtr message_;
  remote_control_test::MockRemotePluginInterface mock_module_;
  RemotePluginInterface::RCPluginEventDispatcher event_dispatcher_;
};

TEST_F(RAManagerTest, AcquireResource_NoAppRegistered_Expect_InUse) {
  // Arrange
  EXPECT_CALL(*mock_service_, GetApplication(kAppId1))
      .WillOnce(Return(ApplicationSharedPtr()));
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  // Act & Assert
  EXPECT_EQ(remote_control::AcquireResult::IN_USE,
            ra_manager.AcquireResource(kModuleType1, kAppId1));
}

TEST_F(RAManagerTest,
       AcquireResource_AppRegisteredAnyHmiLevelResourceFree_Expect_Allowed) {
  // Arrange
  EXPECT_CALL(*mock_service_, GetApplication(kAppId1))
      .WillOnce(Return(mock_app_1_));
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  // Act & Assert
  EXPECT_EQ(remote_control::AcquireResult::ALLOWED,
            ra_manager.AcquireResource(kModuleType1, kAppId1));
}

TEST_F(
    RAManagerTest,
    AcquireResource_AppInAnyHmiLevelWantsToAcquireSameResourceTwice_Expect_Allowed) {
  // Arrange
  EXPECT_CALL(*mock_service_, GetApplication(kAppId1))
      .WillOnce(Return(mock_app_1_));
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  EXPECT_EQ(remote_control::AcquireResult::ALLOWED,
            ra_manager.AcquireResource(kModuleType1, kAppId1));
  EXPECT_CALL(*mock_service_, GetApplication(kAppId1))
      .WillOnce(Return(mock_app_1_));
  // Same app tries to get already acquired resource
  EXPECT_EQ(remote_control::AcquireResult::ALLOWED,
            ra_manager.AcquireResource(kModuleType1, kAppId1));
}

TEST_F(
    RAManagerTest,
    AcquireResource_App2_NotInFULLWantsToGetAcquiredResource_Expect_Rejected) {
  using namespace mobile_apis;
  using namespace hmi_apis;
  using namespace remote_control;
  const HMILevel::eType app_level = HMILevel::eType::HMI_BACKGROUND;
  const AcquireResult::eType expected_result = AcquireResult::REJECTED;
  const Common_RCAccessMode::eType access_mode =
      Common_RCAccessMode::eType::AUTO_ALLOW;
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  CheckResultWithHMILevelAndAccessMode(
      ra_manager, app_level, expected_result, access_mode);
}

TEST_F(
    RAManagerTest,
    AcquireResource_App2_InFULLWantsToGetAcquiredResource_AUTO_DENY_Expect_InUse) {
  using namespace mobile_apis;
  using namespace hmi_apis;
  using namespace remote_control;
  const HMILevel::eType app_level = HMILevel::eType::HMI_FULL;
  const AcquireResult::eType expected_result = AcquireResult::IN_USE;
  const Common_RCAccessMode::eType access_mode =
      Common_RCAccessMode::eType::AUTO_DENY;
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  CheckResultWithHMILevelAndAccessMode(
      ra_manager, app_level, expected_result, access_mode);
}

TEST_F(
    RAManagerTest,
    AcquireResource_App2_InFULLWantsToGetAcquiredResource_AUTO_ALLOW_Expect_Allowed) {
  using namespace mobile_apis;
  using namespace hmi_apis;
  using namespace remote_control;
  const HMILevel::eType app_level = HMILevel::eType::HMI_FULL;
  const AcquireResult::eType expected_result = AcquireResult::ALLOWED;
  const Common_RCAccessMode::eType access_mode =
      Common_RCAccessMode::eType::AUTO_ALLOW;
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  CheckResultWithHMILevelAndAccessMode(
      ra_manager, app_level, expected_result, access_mode);
}

TEST_F(
    RAManagerTest,
    AcquireResource_App2_InFULLWantsToGetAcquiredResource_ASK_DRIVER_Expect_AskDriver) {
  using namespace mobile_apis;
  using namespace hmi_apis;
  using namespace remote_control;
  const HMILevel::eType app_level = HMILevel::eType::HMI_FULL;
  const AcquireResult::eType expected_result = AcquireResult::ASK_DRIVER;
  const Common_RCAccessMode::eType access_mode =
      Common_RCAccessMode::eType::ASK_DRIVER;
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  CheckResultWithHMILevelAndAccessMode(
      ra_manager, app_level, expected_result, access_mode);
}

TEST_F(RAManagerTest,
       AcquireResource_AcquiredModuleIsRejectedForApp2_ExpectApp2Rejected) {
  // Arrange
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  EXPECT_CALL(*mock_service_, GetApplication(kAppId1))
      .WillOnce(Return(mock_app_1_));
  EXPECT_EQ(AcquireResult::ALLOWED,
            ra_manager.AcquireResource(kModuleType1, kAppId1));

  // Act
  ra_manager.OnDriverDisallowed(kModuleType1, kAppId2);

  // Assert
  EXPECT_CALL(*mock_service_, GetApplication(kAppId2))
      .WillOnce(Return(mock_app_2_));
  EXPECT_EQ(AcquireResult::REJECTED,
            ra_manager.AcquireResource(kModuleType1, kAppId2));
}

TEST_F(RAManagerTest,
       AcquireResource_App1OccupiedResourceAndDisconnected_ExpectApp2Allowed) {
  // Arrange
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  ra_manager.SetAccessMode(hmi_apis::Common_RCAccessMode::eType::AUTO_DENY);

  EXPECT_CALL(*mock_service_, GetApplication(kAppId1))
      .WillOnce(Return(mock_app_1_));
  EXPECT_EQ(remote_control::AcquireResult::ALLOWED,
            ra_manager.AcquireResource(kModuleType1, kAppId1));

  // Act
  ra_manager.OnUnregisterApplication(kAppId1);

  // Assert
  EXPECT_CALL(*mock_service_, GetApplication(kAppId2))
      .WillOnce(Return(mock_app_2_));
  EXPECT_EQ(remote_control::AcquireResult::ALLOWED,
            ra_manager.AcquireResource(kModuleType1, kAppId2));
}

TEST_F(RAManagerTest, AskDriver_ExpectDriverConsentRequestSentToHMI) {
  // Arrange
  EXPECT_CALL(*mock_service_, GetNextCorrelationID())
      .WillOnce(Return(kCorrelationId));
  EXPECT_CALL(mock_module_, event_dispatcher())
      .WillOnce(ReturnRef(event_dispatcher_));
  application_manager::MessagePtr result_msg;
  EXPECT_CALL(*mock_service_, SendMessageToHMI(_))
      .WillOnce(SaveArg<0>(&result_msg));
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  AskDriverCallBackPtr ask_driver_callback_ptr =
      utils::MakeShared<remote_control_test::MockAskDriverCallBack>();
  ra_manager.AskDriver(kModuleType1, kAppId1, ask_driver_callback_ptr);
  // Assertions
  const Json::Value message_to_hmi =
      MessageHelper::StringToValue(result_msg->json_message());
  EXPECT_EQ(kAppId1,
            message_to_hmi[json_keys::kParams][json_keys::kAppId].asUInt());
  EXPECT_EQ(kModuleType1,
            message_to_hmi[json_keys::kParams][message_params::kModuleType]
                .asString());
  EXPECT_EQ(functional_modules::hmi_api::get_user_consent,
            message_to_hmi[json_keys::kMethod].asString());
}

TEST_F(RAManagerTest, ForceAcquireResource_ExpectModuleIsAllocatedToAppId) {
  // Arrange
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  // Act
  ra_manager.ForceAcquireResource(kModuleType1, kAppId1);
  // Assert
  EXPECT_EQ(kAppId1, ra_manager.allocated_resources_[kModuleType1]);
}

TEST_F(RAManagerTest,
       OnDriverDisallowed_ModuleIsAbsentInRejectedList_ModuleIsAddedToList) {
  // Arrange
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  // Act
  ra_manager.OnDriverDisallowed(kModuleType1, kAppId1);
  // Assert
  std::vector<std::string> rejected_resources_list =
      ra_manager.rejected_resources_for_application_[kAppId1];
  uint32_t count = std::count(rejected_resources_list.begin(),
                              rejected_resources_list.end(),
                              kModuleType1);
  EXPECT_EQ(1u, count);
}

TEST_F(
    RAManagerTest,
    OnDriverDisallowed_ModuleIsExistsInRejectedList_ModuleIsNotAddedToListAgain) {
  // Arrange
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  // Act
  ra_manager.OnDriverDisallowed(kModuleType1, kAppId1);
  ra_manager.OnDriverDisallowed(kModuleType1, kAppId1);
  // Assert
  std::vector<std::string> rejected_resources_list =
      ra_manager.rejected_resources_for_application_[kAppId1];
  uint32_t count = std::count(rejected_resources_list.begin(),
                              rejected_resources_list.end(),
                              kModuleType1);
  EXPECT_EQ(1u, count);
}

TEST_F(RAManagerTest,
       OnUnregisterApplication_AppWithAllocatedRes_ResourceIsReleased) {
  // Arrange
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  ra_manager.allocated_resources_[kModuleType1] = kAppId1;
  ra_manager.allocated_resources_[kModuleType2] = kAppId2;
  ra_manager.resources_state_[kModuleType1] = ResourceState::BUSY;
  ra_manager.resources_state_[kModuleType2] = ResourceState::BUSY;
  // Act
  ra_manager.OnUnregisterApplication(kAppId1);
  // Assert
  ResourceAllocationManagerImpl::AllocatedResources::const_iterator cait =
      ra_manager.allocated_resources_.find(kModuleType1);
  EXPECT_EQ(ra_manager.allocated_resources_.end(), cait);
  ResourceAllocationManagerImpl::AllocatedResources::const_iterator cait2 =
      ra_manager.allocated_resources_.find(kModuleType2);
  EXPECT_NE(ra_manager.allocated_resources_.end(), cait2);
  EXPECT_EQ(kAppId2, cait2->second);
  ResourceAllocationManagerImpl::ResourcesState::const_iterator csit =
      ra_manager.resources_state_.find(kModuleType1);
  EXPECT_EQ(ra_manager.resources_state_.end(), csit);
  ResourceAllocationManagerImpl::ResourcesState::const_iterator csit2 =
      ra_manager.resources_state_.find(kModuleType2);
  EXPECT_NE(ra_manager.resources_state_.end(), csit2);
  EXPECT_EQ(ResourceState::BUSY, csit2->second);
}

TEST_F(
    RAManagerTest,
    OnUnregisterApplication_AppWithoutAllocatedRes_OtherResourceIsNotReleased) {
  // Arrange
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  ra_manager.allocated_resources_[kModuleType2] = kAppId2;
  ra_manager.resources_state_[kModuleType2] = ResourceState::FREE;
  // Act
  ra_manager.OnUnregisterApplication(kAppId1);
  // Assert
  ResourceAllocationManagerImpl::AllocatedResources::const_iterator cait =
      ra_manager.allocated_resources_.find(kModuleType2);
  EXPECT_NE(ra_manager.allocated_resources_.end(), cait);
  EXPECT_EQ(kAppId2, cait->second);
  ResourceAllocationManagerImpl::ResourcesState::const_iterator csit =
      ra_manager.resources_state_.find(kModuleType2);
  EXPECT_NE(ra_manager.resources_state_.end(), csit);
  EXPECT_EQ(ResourceState::FREE, csit->second);
}

TEST_F(RAManagerTest,
       OnUnregisterApplication_AppWithRejectedRes_ModuleIsNotExistsInList) {
  // Arrange
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  ra_manager.rejected_resources_for_application_[kAppId1] =
      std::vector<std::string>();
  std::vector<std::string>& rejected_res_list =
      ra_manager.rejected_resources_for_application_[kAppId1];
  rejected_res_list.push_back(kModuleType2);
  rejected_res_list.push_back(kModuleType1);
  // Act
  ra_manager.OnUnregisterApplication(kAppId1);
  // Assert
  ResourceAllocationManagerImpl::RejectedResources::const_iterator cit =
      ra_manager.rejected_resources_for_application_.find(kAppId1);
  EXPECT_EQ(ra_manager.rejected_resources_for_application_.end(), cit);
}

TEST_F(RAManagerTest,
       OnUnregisterApplication_AppWithoutRejectedRes_ModuleIsNotExistsInList) {
  // Arrange
  ResourceAllocationManagerImpl ra_manager(mock_module_);
  // Act
  ra_manager.OnUnregisterApplication(kAppId1);
  // Assert
  ResourceAllocationManagerImpl::RejectedResources::const_iterator cit =
      ra_manager.rejected_resources_for_application_.find(kAppId1);
  EXPECT_EQ(ra_manager.rejected_resources_for_application_.end(), cit);
}

}  // namespace remote_control
