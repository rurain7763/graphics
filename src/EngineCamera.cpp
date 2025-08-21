#include "pch.h"
#include "EngineCamera.h"
#include "Input/Input.h"
#include "Time/Time.h"

EngineCamera::EngineCamera()
    : _perspective(true)
    , _zoomRate(1.0f)
    , _zoomSpeed(0.1f)
    , _position(0.0f, 0.0f, -5.0f)
    , _rotation(0.0f)
{
    _prevMousePos = vec2(Input::GetMouseX(), Input::GetMouseY());

    const float nearClip = 0.1f;
    const float farClip = 1000.0f;
    const float aspectRatio = 800.f / 600.f;

    const float fovY = glm::radians(45.0f);
    _perspectiveCamera = CreateRef<PerspectiveCamera>(_position, Forward, fovY, aspectRatio, nearClip, farClip);

    const float orthoHeight = 1.0f;
    const float orthoWidth = orthoHeight * aspectRatio;
    _orthographicCamera = CreateRef<OrthographicCamera>(_position, Forward, -orthoWidth, orthoWidth, -orthoHeight, orthoHeight, nearClip, farClip);
}

void EngineCamera::OnUpdatePerspective(const vec2& moveDelta) {
    // TODO: 이동 속도를 조절할 수 있도록 해야함
    const float speed = 10.0f;

    bool dirty = false;

    if (length2(moveDelta) > 0.0f) {
        vec3 forward = QRotate(_rotation, Forward);
        vec3 right = glm::cross(Up, forward);
        vec2 normalized = normalize(moveDelta);

        _position += vec3(right.x, right.y, right.z) * normalized.x * speed * Time::DeltaTime();
        _position += vec3(forward.x, forward.y, forward.z) * normalized.y * speed * Time::DeltaTime();

        dirty = true;
    }

    vec2 mousePos = vec2(Input::GetMouseX(), Input::GetMouseY());
    vec2 mouseDelta = mousePos - _prevMousePos;

    if (!EpsilonEqual(glm::length2(mouseDelta), 0.f)) {
        mouseDelta = glm::normalize(mouseDelta);

        _rotation.y += mouseDelta.x * glm::pi<float>() * Time::DeltaTime();
        _rotation.x += mouseDelta.y * glm::pi<float>() * Time::DeltaTime();

        glm::clamp(_rotation.x, -glm::half_pi<float>(), glm::half_pi<float>());

        dirty = true;
    }

    _prevMousePos = mousePos;

    if (dirty) {
        _perspectiveCamera->UpdateViewMatrix(_position, QRotate(_rotation, Forward));
    }
}

void EngineCamera::OnUpdateOrthographic(const vec2& moveDelta) {
    // TODO: 이동 속도를 조절할 수 있도록 해야함
    const float speed = 10.0f;

    if (length2(moveDelta) > 0.0f) {
        vec2 normalized = normalize(moveDelta);
        _position += vec3(normalized.x, normalized.y, 0.0f) * speed * Time::DeltaTime();
    }

    const float ms = Input::GetMouseScrollY();
    if (ms != 0.0) {
        _zoomRate += ms * _zoomSpeed;
        if (_zoomRate < 0.1f) {
            _zoomRate = 0.1f;
        }
    }
}

void EngineCamera::OnUpdate() {
    if (!Input::GetMouseButton(MouseButton::Right)) {
        _moving = false;
        return;
    }

    vec2 delta = vec2(0.0f);

    if (Input::GetKey(KeyCode::W)) {
        delta.y = 1.0f;
    }
    else if (Input::GetKey(KeyCode::S)) {
        delta.y = -1.0f;
    }

    if (Input::GetKey(KeyCode::A)) {
        delta.x = -1.0f;
    }
    else if (Input::GetKey(KeyCode::D)) {
        delta.x = 1.0f;
    }

    if (_perspective) {
        OnUpdatePerspective(delta);
    }
    else {
        OnUpdateOrthographic(delta);
    }

    _moving = !EpsilonEqual(glm::length2(delta), 0);
};

Ref<Camera> EngineCamera::GetCurrentCamera() const {
    if (_perspective) {
        return _perspectiveCamera;
    }
    else {
        return _orthographicCamera;
    }
}

void EngineCamera::SetAspectRatio(float aspectRatio) {
    if (_perspective) {
        const vec2 fov = _perspectiveCamera->GetFov();
        const vec2 nearFarClip = _perspectiveCamera->GetNearFarClip();

        _perspectiveCamera->UpdateProjectionMatrix(fov.y, aspectRatio, nearFarClip.x, nearFarClip.y);
    }
    else {
        const vec4 size = _orthographicCamera->GetSize();
        const vec2 nearFarClip = _orthographicCamera->GetNearFarClip();

        float halfHeight = (size.w - size.y) * 0.5f;
        float halfWidth = halfHeight * aspectRatio;

        _orthographicCamera->UpdateProjectionMatrix(-size.x, size.x, -halfHeight, halfHeight, nearFarClip.x, nearFarClip.y);
    }
}

vec2 EngineCamera::GetNearFarClip() const {
	if (_perspective) {
		return _perspectiveCamera->GetNearFarClip();
	}
	else {
		return _orthographicCamera->GetNearFarClip();
	}
}

mat4 EngineCamera::GetViewMatrix() const {
    if (_perspective) {
        return _perspectiveCamera->GetViewMatrix();
    }
    else {
        return _orthographicCamera->GetViewMatrix();
    }
}

mat4 EngineCamera::GetProjectionMatrix() const {
    if (_perspective) {
        return _perspectiveCamera->GetProjectionMatrix();
    }
    else {
        return _orthographicCamera->GetProjectionMatrix();
    }
}