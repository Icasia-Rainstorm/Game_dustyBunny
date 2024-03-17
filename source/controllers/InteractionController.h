#ifndef __INTERACTION_CONTROLLER_H__
#define __INTERACTION_CONTROLLER_H__

#include <cugl/cugl.h>
#include <stdio.h>
#include <box2d/b2_world.h>
#include <box2d/b2_contact.h>
#include <box2d/b2_collision.h>
#include <ctime>
#include <string>
#include <iostream>
#include <sstream>
#include "NetworkController.h"
#include "../helpers/LevelLoader.h" //Need to include in dependency graph
#include "../models/WallModel.h"
#include "../models/PlatformModel.h"
#include "../models/ButtonModel.h"
#include "../models/ExitModel.h"
#include "CharacterController.h"


using namespace cugl::physics2::net;

#define PLAYER_ONE 1
#define PLAYER_TWO 2
#define NOT_PLAYER 0

class InteractionController {
protected:
    /** Network Controller? */
    std::shared_ptr<NetEventController> _netController;
    // TODO: Interaction Controller SHOULD have access to models. Models should be passed in as parameters in initializers.
    std::shared_ptr<CharacterController> _characterControllerA;
    std::shared_ptr<CharacterController> _characterControllerB;
    
    std::vector<std::shared_ptr<PlatformModel>> _platforms;
    std::vector<std::shared_ptr<ButtonModel>> _buttons;
    std::vector<std::shared_ptr<WallModel>> _walls;
    std::shared_ptr<LevelLoader> _level;

    
    struct PlayersContact {
        int bodyOne = NOT_PLAYER;
        int bodyTwo = NOT_PLAYER;
    };
    
    PlayersContact checkContactForPlayer(b2Body* body1, b2Body* body2);
    
public:
    struct PublisherMessage {
          std::string pub_id;
          std::string trigger;
          std::string message;
          std::shared_ptr<std::unordered_map<std::string, std::string>> body;
    };
    
    struct SubscriberMessage {
          std::string pub_id;
          std::string listening_for;
          std::unordered_map<std::string, std::string> actions;
    };
    
    std::queue<PublisherMessage> messageQueue;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<SubscriberMessage>>> subscriptions;
    //subscriptions[pub_id][listening_for]

    /** the TENTATIVE initializer for this (feel free to change), according to our arch spec
     */
    bool init(std::vector<std::shared_ptr<PlatformModel>> platforms,                
              std::shared_ptr<CharacterController> characterA,
              std::shared_ptr<CharacterController> characterB,
              std::vector<std::shared_ptr<ButtonModel>> buttons,
              std::vector<std::shared_ptr<WallModel>> walls,
              std::shared_ptr<LevelLoader> level);
    
    
    //TODO: Make this method protected, but public right now for testing
    void publishMessage(PublisherMessage&& message);
/** 
 * Creates a new subscription
 *
 * @param message  The subscriber message
 *
 * @return true if the publisher exists and the subscriber was added, false otherwise
 */
    bool addSubscription(SubscriberMessage&& message);

    
#pragma mark Collision Handling
    /**
     * Processes the start of a collision
     *
     * This method is called when we first get a collision between two objects.
     * We use this method to test if it is the "right" kind of collision.  In
     * particular, we use it to test if we make it to the win door.
     *
     * @param  contact  The two bodies that collided
     */
    void beginContact(b2Contact* contact);
    
    void endContact(b2Contact* contact);

    /**
     * Handles any modifications necessary before collision resolution
     *
     * This method is called just before Box2D resolves a collision.  We use
     * this method to implement sound on contact, using the algorithms outlined
     * in Ian Parberry's "Introduction to Game Physics with Box2D".
     *
     * @param  contact  The two bodies that collided
     * @param  contact  The collision manifold before contact
     */
    void beforeSolve(b2Contact* contact, const b2Manifold* oldManifold);
    
    /**
     * Handles message subscription processing
     * @param  dt  timestep
     */
    void preUpdate(float dt);
};

#endif /* InteractionController_h */
