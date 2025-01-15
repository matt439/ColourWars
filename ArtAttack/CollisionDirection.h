#ifndef COLLISIONDIRECTION_H
#define COLLISIONDIRECTION_H

/**
     *  This enumeration contains a list of directions that an
     *  object can collide with another object. For example, a
     *  collider object which is colliding with another object
     *  on its top edge would have a collision direction of TOP.
     *
     *  @constant TOP           The top of the object
     *  @constant BOTTOM        The bottom of the object
     *  @constant LEFT          The left of the object
     *  @constant RIGHT         The right of the object
     *  @constant TOP_LEFT      The top left of the object
     *  @constant TOP_RIGHT     The top right of the object
     *  @constant BOTTOM_LEFT   The bottom left of the object
     *  @constant BOTTOM_RIGHT  The bottom right of the object
     *  @constant NONE          No collision
*/
enum collision_direction
{
    TOP,
    BOTTOM,
    LEFT,
    RIGHT,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    NONE,
};

#endif // !COLLISIONDIRECTION_H