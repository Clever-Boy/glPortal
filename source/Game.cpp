#include "Game.hpp"

#include <stdexcept>
#include <string>
#include <cstdio>
#include <iostream>

#include <SDL2/SDL_timer.h>

#include <radix/component/Transform.hpp>
#include <radix/component/Player.hpp>
#include <radix/env/Environment.hpp>
#include <radix/env/ArgumentsParser.hpp>
#include <radix/env/Util.hpp>
#include <radix/map/XmlMapLoader.hpp>
#include <radix/SoundManager.hpp>
#include <radix/core/diag/Throwables.hpp>
#include <radix/system/PlayerSystem.hpp>
#include <radix/system/PhysicsSystem.hpp>
#include <util/sdl/Fps.hpp>
#include "renderer/UiRenderer.hpp"
#include <SDL2/SDL_keyboard.h>

using namespace radix;

namespace glPortal {

Fps Game::fps;

Game::Game() :
  world(window),
  closed(false),
  config(Environment::getConfig()){
  window.setConfig(config);
  window.create("GlPortal");

  try {
    SoundManager::init();
    init();
    loadMap();
  } catch (std::runtime_error &e) {
    Util::Log(Error) << "Runtime Error: " << e.what();
  }
}

void Game::init() {
  if(config.cursorVisibility) {
    window.unlockMouse();
  } else {
    window.lockMouse();
  }
  world.create();
  renderer = std::make_unique<Renderer>(world);
  camera = std::make_unique<Camera>();
  { World::SystemTransaction st = world.systemTransact();
    st.addSystem<PlayerSystem>();
    st.addSystem<PhysicsSystem>();
  }
  nextUpdate = SDL_GetTicks(), lastUpdate = 0, lastRender = 0;

  renderer->setViewport(&window);

  uiRenderer = std::make_unique<UiRenderer>(world, *renderer.get());
}

bool Game::isRunning() {
  return !closed;
}

World* Game::getWorld() {
  return &world;
}

void Game::loadMap() {
  XmlMapLoader mapLoader(world);
  std::string mapPath = config.mapPath;
  if (mapPath.length() > 0) {
    mapLoader.load(mapPath);
  } else {
    mapLoader.load(Environment::getDataDir() + "/maps/n1.xml");
  }
}

void Game::update() {
  int skipped = 0;
  currentTime = SDL_GetTicks();

  while (currentTime > nextUpdate && skipped < MAX_SKIP) {
    nextUpdate += SKIP_TIME;
    skipped++;
  }
  int elapsedTime = currentTime - lastUpdate;
  SoundManager::update(world.getPlayer());
  world.update(TimeDelta::msec(elapsedTime));
  lastUpdate = currentTime;
}

void Game::processInput() {
  window.processEvents();
  if (window.isKeyDown(SDL_SCANCODE_Q)) {
    close();
  }
}

void Game::cleanUp() {
  world.destroy();
  window.close();
}

void Game::render() {
  prepareCamera();

  renderer->render((currentTime-lastRender)/1000., *camera.get());
  uiRenderer->render();

  fps.countCycle();
  window.swapBuffers();
  lastRender = currentTime;
}

void Game::prepareCamera() {
  camera->setPerspective();
  int viewportWidth, viewportHeight;
  window.getSize(&viewportWidth, &viewportHeight);
  camera->setAspect((float)viewportWidth / viewportHeight);
  const Transform &playerTransform = world.getPlayer().getComponent<Transform>();
  Vector3f headOffset(0, playerTransform.getScale().y, 0);
  camera->setPosition(playerTransform.getPosition() + headOffset);
  const Player &playerComponent = world.getPlayer().getComponent<Player>();
  camera->setOrientation(playerComponent.getHeadOrientation());
}

void Game::close() {
  closed = true;
}

} /* namespace glPortal */
