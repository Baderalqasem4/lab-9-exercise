#ifndef DYNAMIC_CAMERA_H
#define DYNAMIC_CAMERA_H

#include <iostream>
#include <limits>

#include "camera.h"

constexpr float INF = std::numeric_limits<float>::infinity();

// AABB in 1D
struct Interval {
    float min;
    float max;

    constexpr bool contains(const Interval& other) const {
        return (min <= other.min) && (max >= other.max);
    }

    constexpr bool contains(float x) const { return (min <= x) && (x <= max); }

    // signedDistance returns a positive smallest distance of x from the interval boundary
    // if it is not contained, else it returns a smallest (in magnitude) negative distance
    // from the interval boundary.
    constexpr float signedDistance(float x) const {
        if (contains(x)) {
            // 0 or negative
            return std::max(x - max, min - x);
        }
        // positive
        return std::max(x - max, min - x);
    }
};

struct DynamicCamera {
    void update(Camera& camera,                           // the camera to update
                const sf::Vector2f& trackedPosWorld,      // world coords of player to track
                const sf::Vector2f& trackedVelocityWorld  // world velocity of player to track
    ) {

        // set camera to its position without lookahead
        camera.setCameraCenter(cameraUnadjustedCenter);
        
        // get players horizontal position as a percent of the window
        float playerPercent = camera.worldToWindowPercent(trackedPosWorld).x;

        // convert lookahead distance from window percentage to world units
        float lookAheadWorld = camera.windowPercentToWorldDist({lookAhead, 0.f}).x;

        // if player moves left make lookahead distance negative
        if (trackedVelocityWorld.x < 0.f) {
            lookAheadWorld = -lookAheadWorld;
        }

        // if player stops moving remove lookahead slowly
        else if (trackedVelocityWorld.x == 0.f) {
            lookAheadWorld = 0.f;
        }

        // smoothly move lookahead towards its target using ema
        lookaheadState += alpha * (lookAheadWorld - lookaheadState);

        // how far the camera needs to move
        float distance = 0.f;

        // if player is beyond the left red line move camera immediately
        if (playerPercent < softZoneX.min) {
            distance = playerPercent - softZoneX.min;
        }

        // if player is past the right red line move camera immediately
        else if (playerPercent > softZoneX.max) {
            distance = playerPercent - softZoneX.max;
        }

        // if player is between the left green and red lines move camera smoothly
        else if (playerPercent < deadZoneX.min) {
            distance = (playerPercent - deadZoneX.min) * alpha;
        }

        // if player is between the right green and red lines move camera smoothly
        else if (playerPercent > deadZoneX.max) {
            distance = (playerPercent - deadZoneX.max) * alpha;
        }

        // if player is between the green lines camera does not need to move

        // convert camera movement from window percentage to world units
        float distanceWorld = camera.windowPercentToWorldDist({distance, 0.f}).x;

        // move camera using the normal displacement and smoothed lookahead
        camera.setCameraCenter(cameraUnadjustedCenter + sf::Vector2f{distanceWorld + lookaheadState, 0.f});

        // update camera position without lookahead
        cameraUnadjustedCenter.x += distanceWorld;

    

        // TODO: (HINTS/outline)
        // collect statistics on the player position and velocity in (normalized, if you like)
        // window coordinates. Compute lookAhead distance in world coords, adjusting for
        // if the player is moving left or right.
        // Now update lookAhead state using an EMA over its previous state and the lookAhead world
        // vector. Then determine if the player is in the dead zone, soft rejection zone, or hard
        // rejection zone. For each, perform EMA on the delta of the camera displacement (ignoring
        // lookahead) appropriately. Next,
        // set the camera center by offsetting the cameraUnadjustedCenter, combining lookAhead.
        // Finally update cameraUnadjustedCenter, but without lookahead.
    }

    float alpha;  // alpha to use for damping of camera position, *as well as* for smoothing of
                  // lookahead offset.
    Interval deadZoneX;  // percentage interval along x-axis to use for dead zone in [0, 1]
    // percentage interval along x-axis to use for soft rejection zone in [0, 1]
    Interval softZoneX = {-INF, INF};
    // (everything beyond softZone is considered hard zone).
    float lookAhead;  // screen-percentage lookahead
    mutable sf::Vector2f
        cameraUnadjustedCenter;      // the camera world center from last frame, ignoring lookahead
    mutable float lookaheadState{};  // the EMA of the accumulated lookahead offset in world coords
};

#endif // DYNAMIC_CAMERA_H
