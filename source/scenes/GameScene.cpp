#include "GameScene.h"

using namespace cugl;
using namespace cugl::physics2::net;

#define STATIC_COLOR Color4::WHITE
#define PRIMARY_FONT "retro"
/** The message to display on a level reset */
#define RESET_MESSAGE "Resetting"

#pragma mark Initializers and Disposer
/**
 * Creates a new game world with the default values.
 *
 * This constructor does not allocate any objects or start the controller.
 * This allows us to use a controller without a heap pointer.
 */
GameScene::GameScene()
	: cugl::Scene2(), _complete(false), _debug(false), _isHost(false) {}

bool GameScene::init(const std::shared_ptr<cugl::AssetManager>& assets,
	const std::shared_ptr<NetEventController> network,
	bool isHost) {
	return GameScene::init(assets, Rect(0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT),
		Vec2(0, CHARACTER_GRAVITY), network, isHost);
}

bool GameScene::init(const std::shared_ptr<cugl::AssetManager>& assets,
	const cugl::Rect rect,
	const std::shared_ptr<NetEventController> network,
	bool isHost) {
	return init(assets, rect, Vec2(0, CHARACTER_GRAVITY), network, isHost);
}

bool GameScene::init(const std::shared_ptr<cugl::AssetManager>& assets,
	const cugl::Rect rect, const cugl::Vec2 gravity,
	const std::shared_ptr<NetEventController> network,
	bool isHost) {
	Size dimen = computeActiveSize();

	if (assets == nullptr) {
		return false;
	}
	else if (!Scene2::init(dimen)) {
		return false;
	}

	state = INGAME;
	_isHost = isHost;
	_network = network;
	_assets = assets;

	_scale = dimen.width == SCENE_WIDTH ? dimen.width / rect.size.width
		: dimen.height / rect.size.height;
	Vec2 offset{ (dimen.width - SCENE_WIDTH) / 2.0f,
				(dimen.height - SCENE_HEIGHT) / 2.0f };

	//CULog("dimen: %f, %f", dimen.width, dimen.height);
	// TODO: add children to the scene, initialize Controllers
	// Create the scene graph
	_worldnode = scene2::SceneNode::alloc();
	_worldnode->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
	_worldnode->setPosition(offset);

	_debugnode = scene2::SceneNode::alloc();
	_debugnode->setScale(_scale); // Debug node draws in PHYSICS coordinates
	_debugnode->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
	_debugnode->setPosition(offset);

	_loadnode = scene2::Label::allocWithText(RESET_MESSAGE,
		_assets->get<Font>(PRIMARY_FONT));
	_loadnode->setAnchor(Vec2::ANCHOR_CENTER);
	_loadnode->setPosition(computeActiveSize() / 2);
	_loadnode->setForeground(STATIC_COLOR);
	_loadnode->setVisible(false);

	_uinode = scene2::SceneNode::alloc();
	_uinode->setContentSize(dimen);
	_uinode->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
	_uinode->addChild(_loadnode);

	_pauseButton = std::dynamic_pointer_cast<scene2::Button>(
		_assets->get<scene2::SceneNode>("pausebutton"));
    _pauseButton -> removeFromParent();
	_pauseButton->addListener([this](const std::string& name, bool down) {
		if (down) {
			_gamePaused = true;
			//			CULog("Pause button hit");
			//			_network->pushOutEvent(PauseEvent::allocPauseEvent(
			//				Vec2(DEFAULT_WIDTH / 2, DEFAULT_HEIGHT / 2), true));
		}
		});
	_uinode->addChild(_pauseButton);
    

    _levelComplete = _assets->get<scene2::SceneNode>("levelcomplete");
    _levelComplete -> removeFromParent();
    _levelComplete -> doLayout(); // Repositions the HUD
    _levelComplete -> setContentSize(dimen);
    _levelComplete -> setVisible(false);
    _levelCompleteReset = std::dynamic_pointer_cast<scene2::Button>(_levelComplete -> getChildByName("options") -> getChildByName("restart"));
    _levelCompleteReset -> deactivate();
    _levelCompleteReset->addListener([this](const std::string& name, bool down) {
        if (down) {
            std::cout << "Well, level complete reset!" << std::endl;
            this -> state = RESET;
        }
    });
    _levelCompleteMenu = std::dynamic_pointer_cast<scene2::Button>(_levelComplete -> getChildByName("options") -> getChildByName("menu"));
    _levelCompleteMenu -> deactivate();
    _levelCompleteMenu->addListener([this](const std::string& name, bool down) {
        if (down) {
            std::cout << "Well, level MENU!" << std::endl;
            // TODO: there is something weird happening here.
#pragma mark WAITING FOR SOLUTION
            _level -> unload();
            _level -> clearRootNode();
            this -> state = QUIT;
        }
    });
    
    _uinode -> addChild(_levelComplete);
    
    
	addChild(_worldnode);
	addChild(_uinode);

	_worldnode->setContentSize(Size(SCENE_WIDTH, SCENE_HEIGHT));

#pragma mark LevelLoader
	// make the getter for loading the map
    constructLevel();

#pragma mark Paints (TODO)
	// TODO: MOVE THIS TO LEVEL LOADER
	Rect rec3(0, 0, 700, 700);
	Rect rec2(0, 0, 600, 600);
	Rect rec1(0, 0, 500, 500);

	std::vector<Vec2> locations1{ {180, 50}, {180, 50}, {180, 50} };
	auto pm = PaintModel::alloc({ rec1, rec2, rec3 }, locations1, _assets, _worldnode, _scale);
	_paintModels.push_back(pm);

	std::vector<Vec2> locations2{ {200, 70}, {200, 70}, {200, 70} };
	auto pm2 = PaintModel::alloc({ rec1, rec2, rec3 }, locations2, _assets, _worldnode, _scale);
	_paintModels.push_back(pm2);


#pragma mark NetEvents
	_network->attachEventType<GrabEvent>();
	_network->attachEventType<PauseEvent>();
    
#pragma mark PlatformWorld
    _platformWorld = _level->getPhysicsWorld();
    _platformWorld->setGravity(gravity);
    activateWorldCollisions(_platformWorld);

#pragma mark Character 1
    constructCharacterA();

#pragma mark Character 2
//        _characterControllerB = CharacterController::alloc({22,8}, 200);
//        _characterControllerB->buildParts(_assets);
//        _characterControllerB->createJoints();
//
//        auto charNodeB = scene2::SceneNode::alloc();
//        _worldnode->addChild(charNodeB);
//        _characterControllerB->linkPartsToWorld(_platformWorld, charNodeB,
//        _scale);

#pragma mark InteractionController
    _interactionController.init({}, _characterControllerA, nullptr, {}, {}, _level, _level -> getLevelJSON());
    
#pragma mark InputControllers
    // TODO: setup the inputController (PlatformInput, from Harry)
    constructInputController(rect);

//	    _camera.init(charNode,_worldnode,1.0f,
//	    std::dynamic_pointer_cast<OrthographicCamera>(getCamera()),
//	    _uinode, 2.0f);
	_active = false;
	_gamePaused = false;
	_complete = false;
	setDebug(false);


#pragma mark AudioController
//	    _audioController = std::make_shared<AudioController>();
//	    _audioController->init(_assets);
//	    _audioController->play("box2DtheWORSTphysicsEngineIEverUsed",
//	    "PhantomLiberty");

//  _camera.setTarget(_characterControllerA->getBodySceneNode());
	_camera.init(_characterControllerA->getBodySceneNode(), _worldnode, 10.0f,
		std::dynamic_pointer_cast<OrthographicCamera>(getCamera()),
		_uinode, 5.0f);
//	CULog("Character Pos: %f, %f", _characterControllerA->getBodySceneNode()->getPositionX(), _characterControllerA->getBodySceneNode()->getPositionY());
	_camera.setZoom(DEFAULT_ZOOM);
    
    

	return true;
}

void GameScene::addPaintObstacles() {
    // TODO: what?
}

/**
 * Activates world collision callbacks on the given physics world and sets the onBeginContact and beforeSolve callbacks
 *
 * @param world the physics world to activate world collision callbacks on
 */
void GameScene::activateWorldCollisions(const std::shared_ptr<physics2::ObstacleWorld>& world) {
	world->activateCollisionCallbacks(true);
	world->onBeginContact = [this](b2Contact* contact) {
		_interactionController.beginContact(contact);
		};
	world->beforeSolve = [this](b2Contact* contact, const b2Manifold* oldManifold) {
		_interactionController.beforeSolve(contact, oldManifold);
		};
	world->onEndContact = [this](b2Contact* contact) {
		_interactionController.endContact(contact);
		};
    
}

#pragma mark DISPOSE
/**
 * Disposes of all (non-static) resources allocated to this mode.
 */
void GameScene::dispose() {
	if (_active) {
        
        // TODO: implement this -now the logic is very unclear and I just clean up everything
        
		removeAllChildren();
        _characterControllerA->dispose();
		_worldnode = nullptr;
		_debugnode = nullptr;
		_winnode = nullptr;
        _level -> unload();
        _level -> clearRootNode();
        _level = nullptr;
        _uinode = nullptr;
        _levelComplete -> removeFromParent();
        _levelComplete = nullptr;
        _levelCompleteMenu = nullptr;
        _levelCompleteReset = nullptr;
        _pauseButton -> removeFromParent();
        _pauseButton = nullptr;
		_complete = false;
		_debug = false;
		//_characterControllerB->dispose();
		Scene2::dispose();
	}
}

void GameScene::setActive(bool value) {
	if (isActive() != value) {
		Scene2::setActive(value);
		if (value) {
			_pauseButton->activate();
			_gamePaused = false;
            
            _inputController = std::make_shared<InputController>();
            _inputController->init(Rect(0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT));
            _inputController->fillHand(_characterControllerA->getLeftHandPosition(),
                _characterControllerA->getRightHandPosition(),
                _characterControllerA->getLHPos(),
                _characterControllerA->getRHPos());
		}
		else {
			_pauseButton->deactivate();
			_pauseButton->setDown(false);
		}
	}

}


#pragma mark RESET

/**
 * Resets the status of the game so that we can play again.
 *
 * This method disposes of the world and creates a new one.
 */
void GameScene::reset() {
	// reload the level
    state = INGAME;

//	CULog("GAME RESET");
    for(auto & pm : _paintModels){
        pm->clear();
    }
	_assets->unload<LevelLoader>(ALPHA_RELEASE_KEY);
	_assets->load<LevelLoader>(ALPHA_RELEASE_KEY, ALPHA_RELEASE_FILE);

	_level = nullptr;
    constructLevel();

	_platformWorld = nullptr;
	_platformWorld = _level->getPhysicsWorld();
    
    //TODO: refactor out adding to scene so we don't need to reinstatiate everything - do this after LevelLoader refactor
    std::vector<std::shared_ptr<PaintModel>> newPaints;
    for (auto & pm : _paintModels){
        auto re_pm = PaintModel::alloc(pm->getPaintFrames(), pm->getLocations(), _assets, _worldnode, _scale);
        newPaints.push_back(re_pm);
    }
    _paintModels = newPaints;

    constructCharacterA();
    
    _levelCompleteReset -> deactivate();
    _levelCompleteMenu -> deactivate();
    _levelComplete -> setVisible(false);
	setComplete(false);
    _pauseButton -> activate();
    _pauseButton -> setVisible(true);
    
	//reload the input
    constructInputController(Rect(0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT));

	//reload interaction controller
	_interactionController.init({}, _characterControllerA, nullptr, {}, {}, _level, _assets->get<JsonValue>(ALPHA_RELEASE_KEY_JSON));
	//reload the camera
	cameraReset();
	//_camera.setTarget(_characterControllerA->getBodySceneNode());

	activateWorldCollisions(_platformWorld);
    
    
    Application::get()->resetFixedRemainder();
}

/**
 * This method takes a grabEvent and processes it.
 */
void GameScene::processGrabEvent(const std::shared_ptr<GrabEvent>& event) {
	// TODO: Waiting for Other Module
}

/**
 * This method takes a pauseEvent and processes it.
 */
void GameScene::processPauseEvent(const std::shared_ptr<PauseEvent>& event) {
	if (event->isPause()) {
		_gamePaused = true;
	}
	else {
		_gamePaused = false;
	}
}

#pragma mark UPDATE METHODS
void GameScene::preUpdate(float dt) {
	if (_level == nullptr) {
		return;
	}
	// _input.update();
	_inputController->update(dt);
	auto character = _inputController->getCharacter();
	for (auto i = character->_touchInfo.begin(); i != character->_touchInfo.end(); i++) {
		i->worldPos = (Vec2)Scene2::screenToWorldCoords(i->position);
	}

	_inputController->fillHand(_characterControllerA->getLeftHandPosition(),
		_characterControllerA->getRightHandPosition(),
		_characterControllerA->getLHPos(),
		_characterControllerA->getRHPos());
	_inputController->process();
    
    
#pragma mark Interaction Resolves Here
    //_interactionController.preUpdate(dt);
    while (!_interactionController.messageQueue.empty()) {
        InteractionController::PublisherMessage publication = _interactionController.messageQueue.front();
        std::cout << "This publication message is (from GameScene update): " << publication.pub_id << " " << publication.trigger << " " << publication.message << "\n";
        // we extract an active publication message from the queue. This message is supposed to match with some subscriber.
        // we query the corresponding subscriber, according to the pub_id and publication message.
        // by using [], we will automatically skip this for loop if there is no such result.
        for (const InteractionController::SubscriberMessage& subMessage : _interactionController.subscriptions[publication.pub_id][publication.message]) {
            std::cout << "This subscribe message is (from GameScene update): " << subMessage.pub_id << " " << subMessage.listening_for << "\n";
            if (subMessage.actions.count("win") > 0) {
                CULog("Winner! - from preUpdate. Setting Complete.");
                setComplete(true);
            }
            if (subMessage.actions.count("fire") > 0) {
                int bottle = std::stoi(subMessage.actions.at("fire"));
                std::cout << "\nFiring bottle <" << bottle << ">\n\n";
                //TODO: Remove after triggering, probably. I think we should keep it as a vector though, not map
                _paintModels[bottle]->trigger();
                // s.actions.at("fire") is the name of the paint bottle obstacle
            }
        }
        _interactionController.messageQueue.pop();
    }
    // Initialize Grabbing Joints
    _interactionController.connectGrabJoint();


	_characterControllerA->moveLeftHand(INPUT_SCALER *
		_inputController->getLeftHandMovement(), _interactionController.leftHandReverse);
	_characterControllerA->moveRightHand(
		INPUT_SCALER * _inputController->getrightHandMovement(), _interactionController.rightHandReverse);
	_inputController->fillHand(_characterControllerA->getLeftHandPosition(),
		_characterControllerA->getRightHandPosition(),
		_characterControllerA->getLHPos(),
		_characterControllerA->getRHPos());
    
    
	// TODO: error handle for loading different levels when we have multiple levels
	_camera.update(dt);
    processPaintCallbacks(dt);
    _camera.setTarget(_characterControllerA->getBodySceneNode());
	//CULog("Character Pos: %f, %f", _characterControllerA->getBodySceneNode()->getPositionX(), _characterControllerA->getBodySceneNode()->getPositionY());
	//_camera.process(ZOOMIN, 0.01);
	//_camera.process(ZOOMOUT, 0.01);
}

void GameScene::processPaintCallbacks(float millis) {
	for (auto& pm : _paintModels) {
		pm->update(_worldnode, millis);
		if (pm->active) {
			_interactionController.detectPolyContact(pm->currentNode(), _scale);
		}
	}
}

void GameScene::postUpdate(float dt) {
}

void GameScene::fixedUpdate(float dt) {
    _platformWorld->update(dt);
    _characterControllerA->update(dt);
    

//    processPauseEvent();
//	if (_network->isInAvailable()) {
//		auto e = _network->popInEvent();
//		if (auto grabEvent = std::dynamic_pointer_cast<GrabEvent>(e)) {
//			// CULog("BIG Grab Event GOT");
//			processGrabEvent(grabEvent);
//		}
//		else if (auto pauseEvent = std::dynamic_pointer_cast<PauseEvent>(e)) {
//			CULog("BIG Pause Event GOT");
//			processPauseEvent(pauseEvent);
//		}
//	}

	// _characterControllerA->getBodySceneNode()->getPositionX() <<
	// _characterControllerA->getBodySceneNode()->getPositionY() << std::endl;
	//_camera.setTarget(_characterControllerA->getBodySceneNode());
}

void GameScene::update(float dt) {
	// deprecated
}

#pragma mark setComplete
void GameScene::setComplete(bool value){
    _complete = value;
    if (value) {
        _pauseButton -> setVisible(false);
        _pauseButton -> deactivate();
        _levelComplete -> setVisible(true);
        _levelCompleteReset -> activate();
        _levelCompleteMenu -> activate();
    }
}

#pragma mark Helper Functions
void GameScene::constructLevel() {
    _level = _assets->get<LevelLoader>(ALPHA_RELEASE_KEY);
    _level->setAssets(_assets);
    _level->setRootNode(_worldnode);
}

void GameScene::constructInputController(const cugl::Rect &rect) {
    _inputController = std::make_shared<InputController>();
    _inputController->init(rect);
    _inputController->fillHand(_characterControllerA->getLeftHandPosition(),
                               _characterControllerA->getRightHandPosition(),
                               _characterControllerA->getLHPos(),
                               _characterControllerA->getRHPos());
}

void GameScene::constructCharacterA() {
    _characterControllerA = CharacterController::alloc(_level->getCharacterPos(), _scale);
    _characterControllerA->buildParts(_assets);
    _characterControllerA->createJoints();
    
    auto charNode = scene2::SceneNode::alloc();
    _worldnode->addChild(charNode);
    //    _characterController->setSceneNode(charNode);
    _characterControllerA->linkPartsToWorld(_platformWorld, charNode, _scale);
}

/**
 * Returns the active screen size of this scene.
 *
 * This method is for graceful handling of different aspect
 * ratios
 */
Size GameScene::computeActiveSize() const {
    Size dimen = Application::get()->getDisplaySize();
    float ratio1 = dimen.width / dimen.height;
    float ratio2 = ((float)SCENE_WIDTH) / ((float)SCENE_HEIGHT);
    if (ratio1 < ratio2) {
        dimen *= SCENE_WIDTH / dimen.width;
    }
    else {
        dimen *= SCENE_HEIGHT / dimen.height;
    }
    return dimen;
}


void GameScene::cameraReset() {
    _camera.init(_characterControllerA->getBodySceneNode(), _worldnode, 10.0f,
        std::dynamic_pointer_cast<OrthographicCamera>(getCamera()),
        _uinode, 5.0f);
    _camera.setMove(false);
    _camera.setZoom(DEFAULT_ZOOM);
    _camera.setInitialStay(INITIAL_STAY + 1);
    _camera.setFinalStay(FINAL_STAY + 1);
}
