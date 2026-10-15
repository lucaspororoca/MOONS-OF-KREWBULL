#include "scripts/hud/camera.h"

Camera::Camera(int viewportW, int viewportH)
    : x(0.0f), y(0.0f),
      viewportW(viewportW), viewportH(viewportH),
      worldW(0), worldH(0), hasBounds(false)
{
}

void Camera::setBounds(int w, int h) {
    worldW = w;
    worldH = h;
    hasBounds = true;
}

void Camera::follow(float targetX, float targetY) {
    x = targetX - viewportW / 2.0f;
    y = targetY - viewportH / 2.0f;

    if (hasBounds) {
        if (x < 0.0f) x = 0.0f;
        if (y < 0.0f) y = 0.0f;
        if (x + viewportW > worldW) x = static_cast<float>(worldW - viewportW);
        if (y + viewportH > worldH) y = static_cast<float>(worldH - viewportH);
    }
}

void Camera::applyTo(SDL_Rect& rect) const {
    rect.x -= static_cast<int>(x);
    rect.y -= static_cast<int>(y);
}