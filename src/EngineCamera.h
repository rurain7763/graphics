#pragma once

#include <Camera.h>

using namespace flaw;

class EngineCamera {
public:
	EngineCamera();

	void OnUpdate();

	Ref<Camera> GetCurrentCamera() const;

	void SetAspectRatio(float aspectRatio);

	vec2 GetNearFarClip() const;
	mat4 GetViewMatrix() const;
	mat4 GetProjectionMatrix() const;

	void SetPosition(const vec3& position);
	const vec3& GetPosition() const { return _position; }
	vec3 GetFront() const { return QRotate(_rotation, Forward); }

	bool IsMoving() const { return _moving; }
	bool IsPerspective() const { return _perspective; }

private:
	void OnUpdatePerspective(const vec2& moveDelta);
	void OnUpdateOrthographic(const vec2& moveDelta);

private:
	bool _perspective;

	Ref<PerspectiveCamera> _perspectiveCamera;
	Ref<OrthographicCamera> _orthographicCamera;

	// orthographic
	float _zoomRate;
	float _zoomSpeed;

	vec3 _position;
	vec3 _rotation;

	bool _moving;
	vec2 _prevMousePos;
};