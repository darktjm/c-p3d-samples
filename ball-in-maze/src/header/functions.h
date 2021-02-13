#pragma once

//Panda3D includes
#include "event.h"
#include "collisionEntry.h"
#include "pandaFramework.h"
#include "mouseWatcher.h"

//My includes
#include "header/globals.h"

//Exit function
void panda_exit(const Event* theEvent, void* data);

//Wall collide handler
void wall_collide_handler(PT(CollisionEntry) entry);

//Ground collide handler
void ground_collide_handler(PT(CollisionEntry) entry);

//Roll task
AsyncTask::DoneStatus roll_func(GenericAsyncTask* task, void* mouseWatcherNode);