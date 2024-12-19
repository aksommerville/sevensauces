# The Secret of the Seven Sauces

Requires [Egg](https://github.com/aksommerville/egg) to build.

## TODO

- [ ] Finish onboarding sprites copied from 20241217
- - [x] Harvest
- - [x] "drop" and "free" usages for hero.
- [x] Fauna parameters.
- [x] Make 'arrow' sprite work for stone too.
- [ ] Sword
- [ ] Grapple
- [ ] Fishpole
- [ ] Letter
- [ ] Arrow should stop at solid tiles and disappear into water -- both very important for stone
- [ ] Freshen tree and trap tiles. Lots are missing, and we have a bunch with no item anymore. And all look bad.
- [ ] Sort sprites before render
- [ ] Doors
- [ ] Transient entities: Vegetables, animals, stones...
- [ ] Shops: Add a stationer, in addition to the ones from 20241217
- [ ] Condensery
- [ ] Kitchen

## Notes re starting over

Second attempt. The first is sevensauces-20241217, I've kept it for reference.

Key differences from first attempt:
- Much larger possible customer set.
- Ingredients have an intrinsic portion size.
- 3 classes of ingredient.
- 4 classes of customer with simple and strict rules.
- Customer set won't be secret. You know at the start of day who will show up tonight.
- Generalize more UI stuff instead of implementing bespoke in each layer.
- Sprite-on-sprite collisions.
- Transient flora and fauna tracked at world level.

Food groups:
- Veg -- Need a few aliases esp at higher densities because you must be able to cook with all unique vegetables.
- - 1: Carrot
- - 2: Potato
- - 3: Cabbage
- - 4: Banana
- - 5: Watermelon
- - 10: Pumpkin
- - 15: Noodles
- - 20: Bread
- - 25: Bouillon
- Meat
- - 1: Rabbit, Rat
- - 2: Cheese
- - 3: Quail
- - 4: Egg
- - 5: Goat
- - 10: Pig
- - 15: Sheep
- - 20: Deer
- - 25: Cow
- Candy
- - 1: Gumdrop
- - 2: Peppermint
- - 3: Licorice
- - 4: Chocolate
- - 5: Butterscotch
- - 10: Buckeye
- - 15: Apple pie
- - 20: Birthday cake
- - 25: Wedding cake

And then of course there's the Secret Sauces. These should be in their own food group, with density always a multiple of 3, and apply equally to all 3 groups.

Customers:
- Man: Satisfied if both veg and meat are present. Ecstatic if veg, meat, and candy are within a certain threshold of each other.
- Elf: Satisfied if veg>meat. Ecstatic if all veg are different.
- Leprechaun: Satisfied if candy present. Ecstatic if candy>veg and candy>meat.
- Werewolf: Satisfied if anything present. Ecstatic if portions exceed population.

A consequence of this approach is that all customers of a given class will react the same way every time.
Is that desirable?
I think so. The game has to be playable, right? So the player can look at her cauldron and customers and predict the outcome exactly.

^ Dissatisfied customers should appear in the field the next day, and you can give them an Apology Card to bring them back, one at a time.

Ingredients go up to 25 servings, and the cauldron holds 15, so we have a capacity of 375.
Start with 4 customers, one of each race.
After each night, each customer can duplicate, plus one of random race if everybody is satisfied.
Maximum count by day: 4, 9, 19, 39, 79, 159, 319, (639).
The most real customers you could have is 319, and the most for scoring purposes is 639 (including additions Sunday evening).
`2**n*5-1` from zero, if we need a straight formula.
