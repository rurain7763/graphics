#pragma once

#include "Core.h"
#include "Math/Math.h"

namespace flaw {
	class Camera {
	public:
		Camera();
		Camera(const vec3& position, const vec3& lookDirection);
		virtual ~Camera() = default;

		void UpdateViewMatrix(const vec3& position, const vec3& lookDirection);

		mat4 GetViewMatrix();
		mat4 GetProjectionMatrix();
		Frustum GetFrustum();

		vec3 GetPosition() const { return _position; }

	protected:
		virtual void UpdateProjectionMatrix() = 0;
		virtual void UpdateFrustum() = 0;

	protected:
		bool _viewDirty;
		bool _projectionDirty;
		bool _frustumDirty;

		mat4 _view;
		mat4 _projection;

		vec3 _position;
		vec3 _lookDirection;

		Frustum _frustrum;
	};

	class PerspectiveCamera : public Camera {
	public:
		PerspectiveCamera(const vec3& position, const vec3& lookDirection, float fovY, float aspectRatio, float nearClip, float farClip);
		~PerspectiveCamera() = default;

		void UpdateProjectionMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

		vec2 GetFov() const { return _fov; }
		float GetAspectRatio() const { return _aspectRatio; }
		vec2 GetNearFarClip() const { return _nearFarClip; }

	protected:
		void UpdateProjectionMatrix() override;
		void UpdateFrustum() override;

	private:
		vec2 _fov;
		float _aspectRatio;
		vec2 _nearFarClip;
	};

	class OrthographicCamera : public Camera {
	public:
		OrthographicCamera(const vec3& position, const vec3& lookDirection, float left, float right, float bottom, float top, float nearClip, float farClip);
		~OrthographicCamera() = default;

		void UpdateProjectionMatrix(float left, float right, float bottom, float top, float nearClip, float farClip);

		vec4 GetSize() const { return vec4(_left, _bottom, _right, _top); }
		vec2 GetNearFarClip() const { return _nearFarClip; }

	protected:
		void UpdateProjectionMatrix() override;
		void UpdateFrustum() override;

	private:
		float _left, _right, _bottom, _top;
		vec2 _nearFarClip;
	};
}