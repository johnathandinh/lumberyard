/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "precompiled.h"
#include <Styling/PseudoElement.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace GraphCanvas
{
    namespace Styling
    {
        void VirtualChildElement::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<VirtualChildElement>()
                ->Version(1)
                ;
        }

        AZ::EntityId VirtualChildElement::Create(const AZ::EntityId& real, const AZStd::string& virtualChildElement)
        {
            AZ::Entity* entity = aznew AZ::Entity;
            entity->AddComponent(new VirtualChildElement(real, virtualChildElement));
            entity->Init();
            entity->Activate();

            return entity->GetId();
        }

        VirtualChildElement::VirtualChildElement()
        {
            // Doesn't seem like it would be possible to serialize std::function, so...
            AZ_Assert(false, "GraphCanvas::Styling::PseudoElements must not be serialized");
        }

        VirtualChildElement::VirtualChildElement(const AZ::EntityId& real, const AZStd::string& virtualChildElement)
            : m_real(real)
        {
            AZStd::string realElement;
            StyledEntityRequestBus::EventResult(realElement, real, &StyledEntityRequests::GetElement);
            AZ_Warning("GraphCanvas", !realElement.empty(), "Can't create a virtual child element on a 'real' parent element with no element name");

            m_parentSelector = Selector::Get(realElement);
            m_virtualChildSelector = Selector::Get(virtualChildElement);
        }

        AZ::EntityId VirtualChildElement::GetStyleParent() const
        {
            return m_real;
        }

        SelectorVector VirtualChildElement::GetStyleSelectors() const
        {
            SelectorVector selectors;
            StyledEntityRequestBus::EventResult(selectors, m_real, &StyledEntityRequests::GetStyleSelectors);
            std::replace(selectors.begin(), selectors.end(), m_parentSelector, m_virtualChildSelector);

            for (const auto& mapPair : m_dynamicSelectors)
            {
                selectors.push_back(mapPair.second);
            }

            return selectors;
        }

        void VirtualChildElement::AddSelectorState(const char* selectorState)
        {
            auto insertResult = m_dynamicSelectors.insert(AZStd::pair<AZ::Crc32, Selector>(AZ::Crc32(selectorState), Selector::Get(selectorState)));

            if (insertResult.second)
            {
                StyleNotificationBus::Event(GetEntityId(), &StyleNotificationBus::Events::OnStyleChanged);
            }
            else
            {
                AZ_Assert(false, "Pushing the same state(%s) onto the selector stack twice. State cannot be correctly removed.", selectorState);
            }
        }

        void VirtualChildElement::RemoveSelectorState(const char* selectorState)
        {
            AZStd::size_t count = m_dynamicSelectors.erase(AZ::Crc32(selectorState));

            if (count != 0)
            {
                StyleNotificationBus::Event(GetEntityId(), &StyleNotificationBus::Events::OnStyleChanged);
            }
        }

        AZStd::string VirtualChildElement::GetElement() const
        {
            return m_virtualChild;
        }

        AZStd::string VirtualChildElement::GetClass() const
        {
            return AZStd::string();
        }

        AZStd::string VirtualChildElement::GetId() const
        {
            return AZStd::string();
        }

        void VirtualChildElement::Activate()
        {
            StyledEntityRequestBus::Handler::BusConnect(GetEntityId());
        }

        void VirtualChildElement::Deactivate()
        {
            StyledEntityRequestBus::Handler::BusDisconnect();
        }

    }
}