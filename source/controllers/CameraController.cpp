#include "CameraController.h"

using namespace cugl;

bool CameraController::init(const std::shared_ptr<cugl::scene2::SceneNode> target, const std::shared_ptr<cugl::scene2::SceneNode> root, float lerp, std::shared_ptr<cugl::OrthographicCamera> camera,
                            std::shared_ptr<scene2::SceneNode> ui, float maxZoom, bool horizontal, bool skipCameraSpan) {
    _target = target;
    _root = root;
    _lerp = lerp;
    _camera = camera;
    _maxZoom = maxZoom;
    _ui = ui;
    _initialStay = 0;
    _finalStay = 0;
    _horizontal = horizontal;
    _levelComplete = false;
    _counter = 0;
    _completed = false;
    _initialUpdate = false;
    _displayed = skipCameraSpan ? true : false;
    _moveToTop = false;
    _moveToLeft = false;
    _state = skipCameraSpan ? 3 : 0; // if skipping camera span is 3 just remain in game play
    _tutorialState = 0;
    _initPosOnce = 0;
    _replay = false;
    return true;
}

/* Now it is a finite state machine */
void CameraController::update(float dt) {
    if (_isTutorial) {
        tutorialUpdate(dt);
        return;
    }
    if (_replay) {
        _state = 0;
        _moveToLeft = false;
        _moveToTop = false;
        _replay = false;
        _displayed = false;
        this->setZoom(_levelCompleteZoom);
    }

    if (!_moveToLeft && _horizontal) {
        _camera->setPosition(Vec2(_camera->getViewport().getMaxX() / (2 * _camera->getZoom()),
                                   _camera->getViewport().getMaxY() / (2 * _camera->getZoom())));
        _moveToLeft = true;
        Vec2 uiPos =
            Vec2(_camera->getPosition().x - _camera->getViewport().getMaxX() / (2 * _camera->getZoom()), _camera->getPosition().y - _camera->getViewport().getMaxY() / (2 * _camera->getZoom()));
        _UIPosition = uiPos;
        _ui->setPosition(uiPos);
    }

    if (!_moveToTop && !_horizontal) {
        _camera->setPosition(Vec2(_root->getSize().width - _camera->getViewport().getMaxX() / (2 * _camera->getZoom()) - 20 + 100,
                                  _root->getSize().height - _camera->getViewport().getMaxY() / (2 * _camera->getZoom())));
        _moveToTop = true;
        Vec2 uiPos =
            Vec2(_camera->getPosition().x - _camera->getViewport().getMaxX() / (2 * _camera->getZoom()) - 20, _camera->getPosition().y - _camera->getViewport().getMaxY() / (2 * _camera->getZoom()));
        _UIPosition = uiPos;
        _ui->setPosition(uiPos);
    }

    switch (_state) {
    // Initial stay
    case 0: {
        _initialStay++;

        if (_initPosOnce == 0 && _horizontal) {
            _initPosOnce++;
            _camera->setPosition(Vec2(_camera->getPosition().x+50, _camera->getPosition().y));
        }
        //        _camera->setPosition(Vec2(_camera->getPosition().x+30, _camera->getPosition().y));
        _camera->update();
        if (cameraStay(INITIAL_STAY)) {
            _state = 1;
            std::cout << " state changed" << std::endl;
        }

        break;
    }
    // Move the camera to the right
    case 1: {
        Vec2 panSpeed = _panSpeed;
        if (_horizontal) {
            if (_camera->getPosition().x <= _root->getSize().width - 500 - _camera->getViewport().getMaxX() / (2 * _camera->getZoom())) {
                _camera->translate(panSpeed.x, panSpeed.y);
                _camera->update();
            } else {
                if (cameraStay(END_STAY))
                    _state = 2;
            }
        } else {
            if (_camera->getPosition().y >= 20 + _camera->getViewport().getMaxY() / (2 * _camera->getZoom())) {
                _camera->translate(panSpeed.x, panSpeed.y);
                _camera->update();
            } else {
                if (cameraStay(END_STAY)) {
                    _state = 2;
                }
            }
        }
        break;
    }
    // Final stay
    case 2: {
        _finalStay++;
        _camera->update();
        if (cameraStay(FINAL_STAY)) {
            _state = 3;
            _displayed = true;
            this->setZoom(getDefaultZoom());
        }
        break;
    }
    // In the gameplay
    case 3: {
        Vec2 cameraPos = Vec2(_camera->getPosition().x, _camera->getPosition().y);
        Vec2 target;
        Vec2 *dst = new Vec2();
        // Lazily track the target using lerp
        target = Vec2(_target->getWorldPosition().x, _target->getWorldPosition().y);
        Vec2::lerp(cameraPos, target, 30 * dt, dst);
        // Make sure the camera never goes outside of the _root node's bounds
        (*dst).x = std::max(std::min(_root->getSize().width - _camera->getViewport().getMaxX() / (2 * _camera->getZoom()), (*dst).x), _camera->getViewport().getMaxX() / (2 * _camera->getZoom()));
        (*dst).y = std::max(std::min(_root->getSize().height - _camera->getViewport().getMaxY() / (2 * _camera->getZoom()), (*dst).y), _camera->getViewport().getMaxY() / (2 * _camera->getZoom()));
        _camera->translate((*dst).x - cameraPos.x, (*dst).y - cameraPos.y);
        delete dst;
        _camera->update();
        if (_levelComplete) {
            _state = 4;
            this->setZoom(_levelCompleteZoom);
        }
        break;
    }
    // Move to the first ending frame and stay
    case 4: {
        Vec2 panSpeed = _panSpeed;
        if (_horizontal) {
            if (_camera->getPosition().x <= _root->getSize().width - 4000 - _camera->getViewport().getMaxX() / (2 * _camera->getZoom())) {
                _camera->translate(panSpeed.x, panSpeed.y);
                _camera->update();
            } else {
                if (cameraStay(END_STAY))
                    _state = 5;
            }
        } else {
            // Camera calibtrate
            _camera->setPosition(Vec2(_root->getSize().width - _camera->getViewport().getMaxX() / (2 * _camera->getZoom()), _camera->getPosition().y));
            if (_camera->getPosition().y >= (1150 + _camera->getViewport().getMaxY()) / (2 * _camera->getZoom())) {
                _camera->translate(panSpeed.x, panSpeed.y);
                _camera->update();
            } else {
                if (cameraStay(END_STAY))
                    _state = 5;
            }
        }
        break;
    }
    // Move to the second ending frame and stay
    case 5: {
        Vec2 panSpeed = _panSpeed;
        if (_horizontal) {
            if (_camera->getPosition().x <= _root->getSize().width - _camera->getViewport().getMaxX() / (2 * _camera->getZoom())) {
                _camera->translate(panSpeed.x, panSpeed.y);
                _camera->update();
            } else {
                if (cameraStay(END_STAY)) {
                    _state = 0;
                    _completed = true;
                }
            }
        } else {
            // Camera calibtrate
            _camera->setPosition(Vec2(_root->getSize().width - _camera->getViewport().getMaxX() / (2 * _camera->getZoom()), _camera->getPosition().y));
            if (_camera->getPosition().y >= _camera->getViewport().getMaxY() / (2 * _camera->getZoom())) {
                _camera->translate(panSpeed.x, panSpeed.y);
                _camera->update();
            } else {
                if (cameraStay(END_STAY)) {
                    _state = 0;
                    _completed = true;
                }
            }
        }
        break;
    }
    }

    //    if(_state == 0 && _horizontal){
    //        std::cout << "runnign something" << " " << _state <<std::endl;
    //
    //        Vec2 uiPos = Vec2(_camera->getPosition().x - _camera->getViewport().getMaxX() / (2 * _camera->getZoom()) + 10000, _camera->getPosition().y - _camera->getViewport().getMaxY() / (2 *
    //        _camera->getZoom())); _UIPosition = uiPos; _ui->setPosition(uiPos);
    //    } else {
    Vec2 uiPos = Vec2(_camera->getPosition().x - _camera->getViewport().getMaxX() / (2 * _camera->getZoom()), _camera->getPosition().y - _camera->getViewport().getMaxY() / (2 * _camera->getZoom()));
    _UIPosition = uiPos;
    _ui->setPosition(uiPos);
    //    }
}

void CameraController::tutorialUpdate(float dt) {
    switch (_tutorialState) {
    // Initialization
    case (0): {
        this->setZoom(getDefaultZoom());
        _displayed = true;
        //_tutorialState = 1;
        break;
    }
    // Scene 1: use finger to move
    case (1): {
        //_tutorialState = 2;
        break;
    }
    // Scene 2: show the paint
    case (2): {
        this->setZoom(0.3);
        //_tutorialState = 3;
        break;
    }
    // Show the platform
    case (3): {
        this->setZoom(0.2);
        break;
    }
    }

    Vec2 cameraPos = Vec2(_camera->getPosition().x, _camera->getPosition().y);
    Vec2 target;
    Vec2 *dst = new Vec2();
    // Lazily track the target using lerp
    target = Vec2(_target->getWorldPosition().x, _target->getWorldPosition().y);
    Vec2::lerp(cameraPos, target, _lerp * dt, dst);
    // Make sure the camera never goes outside of the _root node's bounds
    (*dst).x = std::max(std::min(_root->getSize().width - _camera->getViewport().getMaxX() / (2 * _camera->getZoom()), (*dst).x), _camera->getViewport().getMaxX() / (2 * _camera->getZoom()));
    (*dst).y = std::max(std::min(_root->getSize().height - _camera->getViewport().getMaxY() / (2 * _camera->getZoom()), (*dst).y), _camera->getViewport().getMaxY() / (2 * _camera->getZoom()));
    _camera->translate((*dst).x - cameraPos.x, (*dst).y - cameraPos.y);
    delete dst;
    _camera->update();

    Vec2 uiPos = Vec2(_camera->getPosition().x - _camera->getViewport().getMaxX() / (2 * _camera->getZoom()), _camera->getPosition().y - _camera->getViewport().getMaxY() / (2 * _camera->getZoom()));
    _UIPosition = uiPos;
    _ui->setPosition(uiPos);
}

void CameraController::setZoom(float zoom) {
    float originalZoom = _camera->getZoom();
    // Don't let it be greater than max zoom
    if (zoom > _maxZoom)
        return;
    _camera->setZoom(zoom);
    // CULog("current zoom is: %f", _camera->getZoom());
    //  If this causes the camera to go out of bounds, revert the change
    //  if (_root->getSize().width < _camera->getViewport().getMaxX() / _camera->getZoom() || _root->getSize().height < _camera->getViewport().getMaxY() / _camera->getZoom()) {
    //     _camera->setZoom(originalZoom);
    // }
    //  Scale the UI so that it always looks the same size
    _ui->setScale(1 / _camera->getZoom());
}

void CameraController::addZoom(float zoom) {
    float originalZoom = _camera->getZoom();
    // Don't let it be greater than max zoom
    if (originalZoom + zoom > _maxZoom)
        return;
    float truezoom = std::max(originalZoom + zoom, 0.01f);
    _camera->setZoom(truezoom);
    // If this causes the camera to go out of bounds, revert the change
    // if (_root->getSize().width < _camera->getViewport().getMaxX() / _camera->getZoom() || _root->getSize().height < _camera->getViewport().getMaxY() / _camera->getZoom()) {
    //    _camera->setZoom(originalZoom);
    //}
    // Scale the UI so that it always looks the same size
    _ui->setScale(1 / _camera->getZoom());
}

void CameraController::setTarget(std::shared_ptr<cugl::scene2::SceneNode> target) { _target = target; }

void CameraController::process(int zoomIn, float speed) {
    if (zoomIn == 0)
        return;
    float originalZoom = _camera->getZoom();
    float truezoom;
    if (zoomIn == 1) {
        if (originalZoom + speed > _maxZoom)
            return;
        truezoom = std::min(originalZoom + speed, _maxZoom);
    } else {
        if (originalZoom - speed < 0.25)
            return;
        truezoom = std::max(originalZoom - speed, 0.25f);
    }
    _camera->setZoom(truezoom);
    _ui->setScale(1 / _camera->getZoom());
}

void CameraController::levelComplete() { _levelComplete = true; }

void CameraController::setCamera(std::string selectedLevelKey) {
    _isTutorial = false;
    if (selectedLevelKey == "tutorial") {
        // TODO: Implement Tutorial Zooms (a state machine)
        setMode(true);
        setDefaultZoom(0.4);
        _levelCompleteZoom = DEFAULT_ZOOM;
        _panSpeed = Vec2(30, 0);
    } else if (selectedLevelKey == "level1"|| selectedLevelKey == "level2" ||selectedLevelKey == "level3") {
        this->setMode(true);
        setDefaultZoom(DEFAULT_ZOOM);
        _levelCompleteZoom = DEFAULT_ZOOM;
        _panSpeed = Vec2(30, 0);
    } else if (selectedLevelKey == "doodlejump") {
        setMode(false);
        setDefaultZoom(DEFAULT_ZOOM);
        _levelCompleteZoom = 0.16;
        _panSpeed = Vec2(0, -30);
    } else if (selectedLevelKey == "level4" || selectedLevelKey == "level5" || selectedLevelKey == "level6" || selectedLevelKey == "level10") {
        setMode(false);
        setDefaultZoom(0.2);
        _levelCompleteZoom = 0.16;
        _panSpeed = Vec2(0, -30);
    } else if (selectedLevelKey == "level7") {
        setMode(false);
        setDefaultZoom(0.2);
        _levelCompleteZoom = 0.16;
        _panSpeed = Vec2(0, -30);
    } else if (selectedLevelKey == "level8") {
        setMode(false);
        setDefaultZoom(0.2);
        _levelCompleteZoom = 0.16;
        _panSpeed = Vec2(0, -30);
    } else if (selectedLevelKey == "level9") {
        setMode(false);
        setDefaultZoom(0.2);
        _levelCompleteZoom = 0.16;
        _panSpeed = Vec2(0, -30);
    } else if (selectedLevelKey == "level13") {
        setMode(false);
        setDefaultZoom(0.2);
        _levelCompleteZoom = 0.16;
        _panSpeed = Vec2(0, -30);
    } else if (selectedLevelKey == "paintdrip") {
        setMode(false);
        setDefaultZoom(0.2);
        _levelCompleteZoom = 0.16;
        _panSpeed = Vec2(0, -30);
    }
}

bool CameraController::cameraStay(int time) {
    if (_counter < time) {
        _counter++;
        return false;
    } else {
        _counter = 0;
        return true;
    }
}
