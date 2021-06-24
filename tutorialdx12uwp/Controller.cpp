#include "pch.h"
#include "Controller.h"

using namespace DirectX;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::System;
using namespace winrt::Windows::Devices::Input;

Controller::Controller(CoreWindow const& window) 
	: m_forward{ false }
	, m_backward{false}
	, m_right{false}
	, m_left{false}
	, m_up{false}
	, m_down{false}
	, m_pitch{0.0f}
	, m_yaw{0.0f}
	, m_leftClicked{false}
	
	
{
	m_velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_command = XMFLOAT3(0.0f, 0.0f, 0.0f);
	InitWindow(window);


}

void Controller::InitWindow(CoreWindow const& window) {
	window.KeyDown({ this,&Controller::OnKeyDown });
	window.KeyUp({ this,&Controller::OnKeyUp });
	window.PointerPressed({ this,&Controller::OnMouseClicked });
	window.PointerReleased({ this,&Controller::OnMouseButtonReleased });

	MouseDevice::GetForCurrentView().MouseMoved({ this,&Controller::OnMouseMoved });
	
}

void Controller::OnMouseClicked(winrt::Windows::UI::Core::CoreWindow const& sender,
	winrt::Windows::UI::Core::PointerEventArgs const& args) {

	auto point = args.CurrentPoint();
	auto properties = point.Properties();
	if (properties.IsLeftButtonPressed())
		m_leftClicked = true;

	OutputDebugString(L"Mouse Clicked!\n");
}

void Controller::OnMouseButtonReleased(winrt::Windows::UI::Core::CoreWindow const& /*sender*/,
	winrt::Windows::UI::Core::PointerEventArgs const& /*args*/) {
	
	m_leftClicked = false;


}

void Controller::OnKeyDown(CoreWindow const& /*sender*/, KeyEventArgs const& args) {

	winrt::Windows::System::VirtualKey Key;
	Key = args.VirtualKey();

	if (Key == VirtualKey::W) {
		m_forward = true;
	}
	else if (Key == VirtualKey::S) {
		m_backward = true;
	}
	else if (Key == VirtualKey::A) {
		m_left = true;
	}
	else if (Key == VirtualKey::D) {
		m_right = true;
	}
	else if (Key == VirtualKey::E) {
		m_up = true;
	}
	else if (Key == VirtualKey::Q) {
		m_down = true;
	}
}

void Controller::OnKeyUp(CoreWindow const& /*sender*/, KeyEventArgs const& args) {

	winrt::Windows::System::VirtualKey Key;
	Key = args.VirtualKey();

	if (Key == VirtualKey::W) {
		m_forward = false;
	} 
	else if (Key == VirtualKey::S) {
		m_backward = false;
	}
	else if (Key == VirtualKey::A) {
		m_left = false;
	}
	else if (Key == VirtualKey::D) {
		m_right = false;
	}
	else if (Key == VirtualKey::E) {
		m_up = false;
	}
	else if (Key == VirtualKey::Q) {
		m_down = false;
	}
	

}

void Controller::OnMouseMoved(MouseDevice const& /* */, MouseEventArgs const& args) {
	XMFLOAT2 mouseDelta;
	
	if (!m_leftClicked)
		return;
	

	mouseDelta.x = static_cast<float>(args.MouseDelta().X);
	mouseDelta.y = static_cast<float>(args.MouseDelta().Y);
	if (std::abs(mouseDelta.x) > ControllerConstants::maxMouseDelta || std::abs(mouseDelta.y) > ControllerConstants::maxMouseDelta)
		return;
	XMFLOAT2 rotationDelta;
	rotationDelta.x = mouseDelta.x * ControllerConstants::RotationGain;
	rotationDelta.y = mouseDelta.y * 5*ControllerConstants::RotationGain;
	m_pitch -= rotationDelta.y;
	m_yaw += rotationDelta.x;

	float limit = XM_PI / 2.0f - 0.01f;
	m_pitch = __max(-limit, m_pitch);
	m_pitch = __min(+limit, m_pitch);

	if (m_yaw > XM_PI) {
		m_yaw -= XM_PI * 2.0f;

	}
	else if (m_yaw < -XM_PI) {
		m_yaw += XM_PI * 2.0f;
	}
	
}



float Controller::Pitch() {
	return m_pitch;
}

float Controller::Yaw() {
	return m_yaw;
}

XMFLOAT3 Controller::Velocity() {
	return m_velocity;
}

XMFLOAT3 Controller::LookDirection() {

	XMFLOAT3  lookDirection;
	float r = cosf(m_pitch);
	//float r = 1.0f;
	lookDirection.y = sinf(m_pitch);
	//lookDirection.y = 0;
	lookDirection.z = r * cosf(m_yaw);
	lookDirection.x = r * sinf(m_yaw);

	return lookDirection;

}

void Controller::Update() {
	if (m_forward) {

		m_command.z += 1.0f; // To consider simultaneous keys

	}
	if (m_backward)
		m_command.z -= 1.0f;
	if (m_left)
		m_command.x -= 1.0f;
	if (m_right)
		m_command.x += 1.0f;
	if (m_up)
		m_command.y += 1.0f;
	if (m_down)
		m_command.y -= 1.0f;

	m_velocity.x = m_command.x * cosf(m_yaw) + m_command.z * sinf(m_yaw);
	m_velocity.z = -m_command.x * sinf(m_yaw)+m_command.z* cosf(m_yaw);
	m_velocity.y = m_command.y;
	m_velocity.x = m_velocity.x * ControllerConstants::MovementGain;
	m_velocity.y = m_velocity.y * ControllerConstants::MovementGain;
	m_velocity.z = m_velocity.z * ControllerConstants::MovementGain;

	m_command = XMFLOAT3(0.0f, 0.0f, 0.0f);

	   
}