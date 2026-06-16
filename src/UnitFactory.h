#pragma once

#include <memory>

class Game;
class MoveableUnit;

std::unique_ptr<MoveableUnit> createMoveableUnit(int team, int name, int x, int y, Game* game);
