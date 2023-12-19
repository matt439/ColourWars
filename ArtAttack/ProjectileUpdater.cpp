#include "pch.h"
#include "ProjectileUpdater.h"

//void ProjectileUpdater::update(Projectile& proj)
//{
//    float dt = this->get_dt();
//    //gravity
//    proj.alter_velocity_y(proj.get_gravity() * dt);
//    if (proj.get_velocity_y() > projectile_consts::MAX_VELOCITY.y)
//    {
//        proj.set_velocity_y(projectile_consts::MAX_VELOCITY.y);
//    }
//    else if (proj.get_velocity_y() < -projectile_consts::MAX_VELOCITY.y)
//    {
//        proj.set_velocity_y(-projectile_consts::MAX_VELOCITY.y);
//    }
//
//    //wind resistance
//    float wind_resistance = proj.get_wind_resistance();
//    if (abs(proj.get_velocity_x()) > wind_resistance * 4.0f)
//    {
//        //right
//        if (proj.get_velocity_x() > 0.0f)
//        {
//            proj.alter_velocity_x(-wind_resistance * dt);
//        }
//        //left
//        else
//        {
//            proj.alter_velocity_x(wind_resistance * dt);
//        }
//    }
//    if (proj.get_velocity_x() > projectile_consts::MAX_VELOCITY.x)
//    {
//        proj.set_velocity_x(projectile_consts::MAX_VELOCITY.x);
//    }
//    else if (proj.get_velocity_x() < -projectile_consts::MAX_VELOCITY.x)
//    {
//        proj.set_velocity_x(-projectile_consts::MAX_VELOCITY.x);
//    }
//
//    //rotate
//    if (proj.get_rotation_origin() != rotation_origin::NO_ROTATION)
//    {
//        proj.set_rotation(
//            atan2f(proj.get_velocity_y(), proj.get_velocity_x()));
//    }
//
//    //displacement
//    proj.set_dx_x(proj.get_velocity_x() * dt);
//    proj.set_dx_y(proj.get_velocity_y() * dt);
//
//    if (proj.get_time() > proj.get_delete_timer())
//    {
//        proj.set_for_deletion(true);
//    }
//    proj.alter_time(dt);
//}