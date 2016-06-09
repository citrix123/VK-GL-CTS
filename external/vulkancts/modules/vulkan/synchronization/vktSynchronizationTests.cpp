/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Synchronization tests
 *//*--------------------------------------------------------------------*/

#include "vktTestGroupUtil.hpp"
#include "vktSynchronizationTests.hpp"
#include "vktSynchronizationSmokeTests.hpp"
#include "vktSynchronizationBasicFenceTests.hpp"
#include "vktSynchronizationBasicSemaphoreTests.hpp"
#include "vktSynchronizationBasicEventTests.hpp"
#include "vktSynchronizationOperationSingleQueueTests.hpp"
#include "vktSynchronizationOperationMultiQueueTests.hpp"

#include "deUniquePtr.hpp"

namespace vkt
{
namespace synchronization
{

namespace
{

void createBasicTests (tcu::TestCaseGroup* group)
{
	group->addChild(createBasicFenceTests	 (group->getTestContext()));
	group->addChild(createBasicSemaphoreTests(group->getTestContext()));
	group->addChild(createBasicEventTests	 (group->getTestContext()));
}

void createOperationTests (tcu::TestCaseGroup* group)
{
	group->addChild(createSynchronizedOperationSingleQueueTests(group->getTestContext()));
	group->addChild(createSynchronizedOperationMultiQueueTests (group->getTestContext()));
}

void createChildren (tcu::TestCaseGroup* group)
{
	tcu::TestContext& testCtx = group->getTestContext();

	group->addChild(createSmokeTests(testCtx));
	group->addChild(createTestGroup	(testCtx, "basic", "Basic synchronization tests", createBasicTests));
	group->addChild(createTestGroup	(testCtx, "op", "Synchronization of a memory-modifying operation", createOperationTests));
}

} // anonymous

tcu::TestCaseGroup* createTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "synchronization", "Synchronization tests", createChildren);
}

} // synchronization
} // vkt
