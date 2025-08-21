#include "pch.h"
#include "Camera.h"

namespace flaw {
	Camera::Camera()
		: _viewDirty(false)
		, _projectionDirty(false)
		, _frustumDirty(false)
		, _view(1.0f)
		, _projection(1.0f)
		, _position(0.0f, 0.0f, 0.0f)
		, _lookDirection(Forward)
	{
		_view = LookAt(_position, _position + _lookDirection, Up);
	}

	Camera::Camera(const vec3& position, const vec3& lookDirection)
		: _viewDirty(true)
		, _projectionDirty(true)
		, _frustumDirty(true)
		, _view(1.0f)
		, _projection(1.0f)
		, _position(position)
		, _lookDirection(lookDirection)
	{
		_view = LookAt(_position, _position + _lookDirection, Up);
	}

	void Camera::UpdateViewMatrix(const vec3& position, const vec3& lookDirection) {
		_position = position;
		_lookDirection = lookDirection;

		_viewDirty = true;
		_frustumDirty = true;
	}

	mat4 Camera::GetViewMatrix() {
		if (_viewDirty) {
			_view = LookAt(_position, _position + _lookDirection, Up);
			_viewDirty = false;
		}

		return _view;
	}

	mat4 Camera::GetProjectionMatrix() {
		if (_projectionDirty) {
			UpdateProjectionMatrix();
			_projectionDirty = false;
		}

		return _projection;
	}

	Frustum Camera::GetFrustum() {
		if (_frustumDirty) {
			UpdateFrustum();
			_frustumDirty = false;
		}

		return _frustrum;
	}

	PerspectiveCamera::PerspectiveCamera(const vec3& position, const vec3& lookDirection, float fovY, float aspectRatio, float nearClip, float farClip)
		: Camera(position, lookDirection)
		, _fov(GetFovX(fovY, aspectRatio), fovY)
		, _aspectRatio(aspectRatio)
		, _nearFarClip(nearClip, farClip)
	{
		UpdateProjectionMatrix();
		UpdateFrustum();
	}

	void PerspectiveCamera::UpdateProjectionMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
		_fov = vec2(GetFovX(fovY, aspectRatio), fovY);
		_aspectRatio = aspectRatio;
		_nearFarClip = vec2(nearClip, farClip);

		_projectionDirty = true;
		_frustumDirty = true;
	}

	void PerspectiveCamera::UpdateProjectionMatrix() {
		_projection = Perspective(_fov.y, _aspectRatio, _nearFarClip.x, _nearFarClip.y);
	}

	void PerspectiveCamera::UpdateFrustum() {
		CreateFrustum(_fov.x, _fov.y, _nearFarClip.x, _nearFarClip.y, _position, _lookDirection, _frustrum);
	}

	OrthographicCamera::OrthographicCamera(const vec3& position, const vec3& lookDirection, float left, float right, float bottom, float top, float nearClip, float farClip)
		: Camera(position, lookDirection)
		, _left(left)
		, _right(right)
		, _bottom(bottom)
		, _top(top)
		, _nearFarClip(nearClip, farClip)
	{
		UpdateProjectionMatrix();
		UpdateFrustum();
	}

	void OrthographicCamera::UpdateProjectionMatrix(float left, float right, float bottom, float top, float nearClip, float farClip) {
		_left = left;
		_right = right;
		_bottom = bottom;
		_top = top;
		_nearFarClip = vec2(nearClip, farClip);

		_projectionDirty = true;
		_frustumDirty = true;
	}

	void OrthographicCamera::UpdateProjectionMatrix() {
		_projection = Orthographic(_left, _right, _bottom, _top, _nearFarClip.x, _nearFarClip.y);
	}

	void OrthographicCamera::UpdateFrustum() {
		// TODO: Implement orthographic frustrum
	}
}