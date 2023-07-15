///////////////////////////////////////////////////////
//                                                   //
// Original code copyright (c) 2019 Ian Eborn.       //
// http://thaumaturge-art.com                        //
// Ported to C++ 2023 by Thomas J. Moore.  All of my //
// work is in the Public Domain, except as required  //
// by compliance with the license on the original    //
// code, stated below.                               //
//                                                   //
// Licensed under the MIT license.                   //
// See "FinalGame/codeLicense".txt, or               //
// https://opensource.org/licenses/MIT               //
//                                                   //
///////////////////////////////////////////////////////

#pragma once
#include <panda3d/animControlCollection.h>
#include <initializer_list>

class GameObject {
  public:
    GameObject(LPoint3 pos, const std::string &model_name,
	       std::initializer_list<std::string> model_anims, int max_health,
	       int max_speed, const std::string &collider_name);
    virtual ~GameObject();
    void update(double dt);
    void alter_health(int d_health);
    const NodePath &get_actor() { return actor; }
  protected:
    NodePath actor;
    AnimControlCollection anims;
    AnimControl *stand_anim, *walk_anim;
    int max_health, health, max_speed;
    LVector3 velocity;
    PN_stdfloat acceleration;
    bool walking;
    NodePath collider;
};

class Player : public GameObject {
  public:
    Player();
    void update(bool key_map[], double dt);
};

class Enemy : public GameObject {
  public:
    void update(Player &player, double dt);
  protected:
    Enemy(LPoint3 pos, const std::string &model_name,
	  std::initializer_list<std::string> model_anims, int max_health,
	  int max_speed, const std::string &collider_name);
    virtual void run_logic(Player &player, double dt) = 0;
    int score_value;
    AnimControl *attack_anim, *die_anim, *spawn_anim;
};

class WalkingEnemy : public Enemy {
  public:
    WalkingEnemy(LPoint3 pos);
  protected:
    void run_logic(Player &player, double dt);
    PN_stdfloat attack_distance;
    LVector2 y_vector;
};

class TrapEnemy : public Enemy {
  public:
    TrapEnemy(LPoint3 pos);
    int move_direction;
    bool ignore_player;
  protected:
    void run_logic(Player &player, double dt);
    bool move_in_x;
};