#include <libdragon.h>
#include <glm/gtc/matrix_transform.hpp>
#include "charactercontroller.hpp"

#include "cube.hpp"

CharacterController::CharacterController(btDiscreteDynamicsWorld *dyn_world, glm::vec3 start_pos) :
    dyn_world(dyn_world)
{
    this->type = GOBJ_PLAYER;
    float mass = 100.0f;
    this->speed = 10.0f;

    phys = new PhysMesh(mass, btVector3(1,1,1), dyn_world, btVector3(start_pos.x, start_pos.y, start_pos.z),
                        0xf, 0xf);
    phys->body->setAngularFactor(0.0f);
    phys->body->setActivationState(DISABLE_DEACTIVATION);
    phys->body->setUserPointer(this);

    model = new Model;

    model->verts = mesh_vertices;
    model->commands = mesh_commands;
    model->commands_size = mesh_commands_length;
    model->verts_size = mesh_vertices_length; 
    model->initalize();

    /* Set player state */
    injump = false;

    /* Set camera position */
    camera_pos = start_pos - glm::vec3(0.0f, -5.0f, -3.0f);
}

void CharacterController::update(controller_data &data)
{
    btTransform player_transform = phys->body->getWorldTransform(); 
    btVector3 player_position = player_transform.getOrigin();

    float right_value = (float)data.c[0].x / 128.0f;
    float fwd_value = -(float)data.c[0].y / 128.0f;

    glm::vec3 cam_fwd = camera_pos - glm::vec3(player_position[0], player_position[1], player_position[2]);
    cam_fwd.y = 0;
    cam_fwd = glm::normalize(cam_fwd);
    glm::vec3 cam_right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), cam_fwd));

    glm::vec3 analog = fwd_value*cam_fwd + right_value*cam_right;

    btVector3 old_velocity = phys->body->getLinearVelocity();
    
    if(glm::length(analog) > 0.01f)
    {
        glm::vec3 analog_dir = glm::normalize(analog);

        float amount = glm::radians(10.0f);
        btQuaternion new_rot(-right_value*amount, 0, 0);

        player_transform.setRotation((player_transform.getRotation() * new_rot));
        phys->body->setWorldTransform(player_transform);
        phys->body->setLinearVelocity(btVector3(speed*analog_dir.x, old_velocity[1], speed*analog_dir.z));
        phys->body->clearForces();
    }
    else
    {
        phys->body->setLinearVelocity(btVector3(0.0f, old_velocity[1], 0.0f));
        phys->body->clearForces();
    }

    /* Raycast to check if player is on ground */
    btVector3 check_position = player_position + btVector3(0, -2, 0);
    btCollisionWorld::ClosestRayResultCallback ray_callback(player_position, check_position);
    /* Only check for collisions with the ground */
    ray_callback.m_collisionFilterMask = 2;
    ray_callback.m_collisionFilterGroup = 2;
    dyn_world->rayTest(player_position, check_position, ray_callback);

    /* Add a jump */
    if(!injump && ray_callback.hasHit() && data.c[0].A)
    {
        btVector3 velocity = phys->body->getLinearVelocity();
        velocity.setY(12.0f);
        phys->body->setLinearVelocity(velocity);
        phys->body->clearForces();
        phys->body->setWorldTransform(player_transform);
        injump = true;
    }

    /* only reset injump value once the player has let go A and touched the ground */ 
    if(ray_callback.hasHit() && injump && !data.c[0].A)
    {
        injump = false;
    }

    btTransform t = player_transform;
    t.setOrigin(btVector3(0,0,0));
    btVector3 fwd = t(btVector3(1.0f, 0.0f, 0.0f));
    glm::vec3 player_pos(player_position[0], player_position[1], player_position[2]);
    glm::vec3 player_fwd(fwd[0], fwd[1], fwd[2]);
    /* Calculate camera position based on new projected position */
    glm::vec3 new_cam_pos = player_pos - (5.0f*player_fwd + glm::vec3(0.0f, -5.0f, 0.0f));

    camera_pos = glm::mix(camera_pos, new_cam_pos, 0.1f);
}

glm::mat4 CharacterController::getViewMatrix()
{
    btTransform player_transform = phys->body->getWorldTransform(); 
    btVector3 player_position = player_transform.getOrigin();
    glm::vec3 player_pos(player_position[0], player_position[1], player_position[2]);

    return glm::lookAt(camera_pos, player_pos, glm::vec3(0.0f, 1.0f, 0.0f));
}
