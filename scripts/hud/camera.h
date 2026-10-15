#ifndef CAMERA_H
#define CAMERA_H

#include <SDL2/SDL.h>

class Camera {
public:
    Camera(int viewportW, int viewportH);

    void follow(float targetX, float targetY);
    void setBounds(int worldW, int worldH);

    int getX() const { return static_cast<int>(x); }
    int getY() const { return static_cast<int>(y); }

    void applyTo(SDL_Rect& rect) const;

private:
    float x, y;
    int viewportW, viewportH;
    int worldW, worldH;
    bool hasBounds;
};

#endif // CAMERA_H