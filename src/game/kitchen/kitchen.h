/* kitchen.h
 * Model for the nocturnal phase of gameplay.
 */
 
#ifndef KITCHEN_H
#define KITCHEN_H

struct kitchen {
  int TODO;
};

void kitchen_del(struct kitchen *kitchen);
struct kitchen *kitchen_new();

void kitchen_commit_to_session(struct kitchen *kitchen);

#endif
