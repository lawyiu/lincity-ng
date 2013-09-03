/* ---------------------------------------------------------------------- *
 * fire.c
 * This file is part of lincity.
 * Lincity is copyright (c) I J Peters 1995-1997, (c) Greg Sharp 1997-2001.
 * (c) Corey Keasling, 2004
 * ---------------------------------------------------------------------- */


#include "fire.h"

FireConstructionGroup fireConstructionGroup(
    "Fire",
     FALSE,                     /* need credit? */
     GROUP_FIRE,
     GROUP_FIRE_SIZE,
     GROUP_FIRE_COLOUR,
     GROUP_FIRE_COST_MUL,
     GROUP_FIRE_BUL_COST,
     GROUP_FIRE_FIREC,
     GROUP_FIRE_COST,
     GROUP_FIRE_TECH,
     GROUP_FIRE_RANGE
);

Construction *FireConstructionGroup::createConstruction(int x, int y, unsigned short type) {
    return new Fire(x, y, type);
}

void Fire::update()
{
    int i;
    /* this so we don't get whole blocks changing in one go. */
    if (burning_days == 0)
        burning_days = rand() % (FIRE_LENGTH / 5);

    if (burning_days > FIRE_LENGTH)
    {
        //is_burning = false;
        if (smoking_days == 0)   /* rand length here also */
            smoking_days = rand() % (AFTER_FIRE_LENGTH / 6);
        smoking_days++;
        if (world(x,y)->flags & FLAG_FIRE_COVER)
            smoking_days += 4;
        if (smoking_days > AFTER_FIRE_LENGTH)
            ConstructionManager::submitRequest
                (
                    new ConstructionDeletionRequest(this)
                );
        else if (smoking_days > (3 * AFTER_FIRE_LENGTH) / 4)
            type = CST_FIRE_DONE4;
        else if (smoking_days > (2 * AFTER_FIRE_LENGTH) / 4)
            type = CST_FIRE_DONE3;
        else if (smoking_days > (AFTER_FIRE_LENGTH) / 4)
            type = CST_FIRE_DONE2;
        else
            type = CST_FIRE_DONE1;
        return;
    }

    burning_days++;
    if (world(x,y)->flags & FLAG_FIRE_COVER)
            burning_days += 4;
    days_before_spread--;
    if( !(flags & FLAG_IS_GHOST) )
    {   world(x,y)->pollution++;}
    if (real_time > anim)
    {
        anim = real_time + FIRE_ANIMATION_SPEED;
        switch (type)
        {
            case (CST_FIRE_1):
                type = CST_FIRE_2;
            break;
            case (CST_FIRE_2):
                type = CST_FIRE_3;
            break;
            case (CST_FIRE_3):
                type = CST_FIRE_4;
            break;
            case (CST_FIRE_4):
                type = CST_FIRE_5;
            break;
            default ://case (CST_FIRE_5):
                type = CST_FIRE_1;
            break;
        }
    }
    if (days_before_spread == 0)
    {
        days_before_spread = FIRE_DAYS_PER_SPREAD;
        if ((rand() % 20) == 1)
        {
            i = rand() % 4;
            switch (i)
            {
                case (0):
                    do_random_fire(x - 1, y, 0);
                break;
                case (1):
                    do_random_fire(x, y - 1, 0);
                break;
                case (2):
                    do_random_fire(x + 1, y, 0);
                break;
                case (3):
                    do_random_fire(x, y + 1, 0);
                break;
            }
        }
    }
}

void Fire::report()
{
    int i = 0;

    mps_store_sd(i++,constructionGroup->name,ID);
    i++;
    mps_store_sd(i++,"Air Pollution",world(x,y)->pollution);
    if (burning_days<FIRE_LENGTH)
        mps_store_sddp(i++,"burnt down",burning_days,FIRE_LENGTH);
    else
        mps_store_sddp(i++,"degraded",smoking_days,AFTER_FIRE_LENGTH);
    //list_commodities(&i);

}
/** @file lincity/modules/fire.cpp */

