#pragma once
#include "pch.h"

namespace ControllerConstants {
	static const float RotationGain = 0.001f;
	static const float MovementGain = 4.0f;
	static const float maxMouseDelta = 500.0f;
}

class Controller {

public:

	Controller(winrt::Windows::UI::Core::CoreWindow const& window);
	void InitWindow(winrt::Windows::UI::Core::CoreWindow const& window);

	void Update();

	DirectX::XMFLOAT3 Velocity();
	DirectX::XMFLOAT3 LookDirection();

	float Yaw();
	float Pitch();


private:

	void OnKeyDown(winrt::Windows::UI::Core::CoreWindow const& sender,
		winrt::Windows::UI::Core::KeyEventArgs const& args);

	void OnKeyUp(winrt::Windows::UI::Core::CoreWindow const& sender,
		winrt::Windows::UI::Core::KeyEventArgs const& args);

	void OnMouseMoved(
		winrt::Windows::Devices::Input::MouseDevice const& mouseDevice,
		winrt::Windows::Devices::Input::MouseEventArgs const& args);

	void OnMouseClicked(winrt::Windows::UI::Core::CoreWindow const& sender,
		winrt::Windows::UI::Core::PointerEventArgs const& args);

	void OnMouseButtonReleased(winrt::Windows::UI::Core::CoreWindow const& sender,
		winrt::Windows::UI::Core::PointerEventArgs const& args);
	

	bool m_forward;
	bool m_backward;
	bool m_left;
	bool m_right;
	bool m_up;
	bool m_down;
	float m_pitch;
	float m_yaw;

	bool m_leftClicked;
	DirectX::XMFLOAT3 m_velocity;

	DirectX::XMFLOAT3 m_command;
	

};