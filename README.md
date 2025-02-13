# The Secret of the Seven Sauces

Requires [Egg](https://github.com/aksommerville/egg) to build.

## TODO

- [x] Losses are reappearing incorrectly.
- [x] Shops sometimes showing an incorrect texture by item description, when empty slot focussed.
- - Seems to happen reliably on the second day, never on the first.
- [x] It's infuriating when animals get cornered in a door, then the door warps me when I try to catch them.
- - Can we make fauna not step on doors? Or change the door trigger to wait until you hit the other side, somehow?
- - Or prevent fauna from entering the outermost cell of map? That might be the cheapest fix.
- [x] Stepping into the cheat room, the first couple items you touch don't trigger. Feels like timing, is there a blackout period?
- [x] !!! Getting texture faults pretty reliably at the first evening. Implement recording in Egg first to help diagnose. Probably need to rewrite graf/texcache.
- [ ] Arrow should stop at solid tiles and disappear into water -- both very important for stone
- [x] Sort sprites before render
- [x] Doors
- [x] Money
- [x] Earn money each day, say $1/customer?
- [x] Transient entities: Vegetables, animals, stones...
- [x] Shops: Add a stationer, in addition to the ones from 20241217
- [ ] Talking NPCs.
- [ ] Condensery
- [ ] Kitchen: Decorative stew content: Show the selected items bobbing and drifting. And random bubbles, etc.
- [ ] Kitchen: Advice.
- [ ] Kitchen: Require confirmation at Ready for obviously incorrect stews. Eg no ingredients, or poison included.
- [ ] Should we give Dot some of her Full Moon toys too?
- - YES: Broom, Umbrella, Bell
- - NO: Chalk, Violin, Hat, Feather, Pitcher
- - ALREADY DONE: Shovel, Seeds, Cheese, Gold
- - MAYBE: Wand, Snowglobe, Matches, Compass.
- - - Wand wouldn't be for casting spells; it would just be a weapon.
- - - Matches probably not, since we don't (and won't) have item quantities.
- - - Compass probably not, there's no buried treasure and no place for narrative guidance.
- [ ] Picking up fauna is not obvious. Need some indication immediately when the clock starts ticking.
- [ ] Release grapple over water. What should happen? What we have is not ok.
- [ ] Fishing odds varying geographically. And maybe different kinds of fish? And maybe you can fish one spot out?
- [ ] Fishing challenges:
- - [ ] Parachute
- - [ ] Machine gun
- - [ ] Spell of darkness
- - [ ] Cat
- [ ] An eighth bonus quest: Rescue the Princess. No sauce for winning, but the Princess will show up as a customer.
- - That way, we have a max of 23 instead of 22 -- exactly the capacity we can render.
- [ ] Review shop prices. I just made up numbers off the cuff, no logic behind them.
- [ ] EOD: Show money earned (and move that into stew).
- [x] Kitchen and shop: Create widgets for empty inventory slots. It's disconcerting when you have a lot of gaps.
- [ ] Teleport shop in the village: 4 remote points but you have to reach them first. Once all 4 opened, you get daily income from the shop.
- [ ] Door transitions
- [ ] Assistance near home:
- - Reveal the Daily Special.
- - Tell composition of tonight's party.
- - Recommend a recipe.
- [ ] Beef up the bow a little. As is, the grapple is actually a much better hunting weapon.

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
Also Poison is technically a food group. It overrides everything except gross quantity: If any Poison was present, anybody that eats it dies.
TODO: Would there ever be a reason to actually use Poison? Nothing springs to mind.
Maybe perishable things like Fish turn into Poison if they linger in inventory too long?

Customers:
- Man: Satisfied if both veg and meat are present. Ecstatic if veg, meat, and candy are within a certain threshold of each other.
- Rabbit: Satisfied if veg>=meat. Ecstatic if all veg are different.
- Octopus: Satisfied if candy present. Ecstatic if candy>veg and candy>meat.
- Werewolf: Satisfied if anything present. Ecstatic if portions exceed population.

There's another race called Princess.
I'm planning for there to be just one Princess, a side quest where you have to rescue her.
Because she's the reward for the side quest, she's super easy to please: Always satisfied as long as she's fed and not poisoned.

A consequence of this approach is that all customers of a given class will react the same way every time.
Is that desirable?
I think so. The game has to be playable, right? So the player can look at her cauldron and customers and predict the outcome exactly.

^ Dissatisfied customers should appear in the field the next day, and you can give them an Apology Card to bring them back, one at a time.

Ingredients go up to 25 servings, and the cauldron holds 15, so we have a capacity of 375.
UPDATE: The Secret Sauces are each worth 126. So in theory the richest stew is 1107 helpings. We should add a bonus for going over 1k, which will be nearly impossible.
Start with 4 customers, one of each race.
After each night, each customer can duplicate, plus one of random race if everybody is satisfied.
Maximum count by day: 4, 9, 19, 39, 79, 159, 319, (639).
The most real customers you could have is 319, and the most for scoring purposes is 639 (including additions Sunday evening).
`2**n*5-1` from zero, if we need a straight formula.

The limit for each food group appears to be 519 (7 sauces, ie `7*42 + 25*9`).
It won't necessarily be possible to get 9 of the 25-point ingredients (eg I'm picturing Wedding Cake can only be got once per day).

-------------------------
2025-02-10
I'm not crazy about the exponential growth. It just feels weird, human intuition isn't great with exponential series.
A radical change of dinner rules:
 - Start with 4 as today, but add no more than 3 per night.
 - Including the Princess, that yields a maximum of 23 -- exactly what the small dining room can accomodate.
 - +1 if all customers are satisfied, as today.
 - +1 for a rule depending on which race has the most members:
 - - Man: Natural veg, meat, and candy counts identical and nonzero.
 - - Rabbit: Natural veg>meat and veg>candy.
 - - Octopus: Natural candy>veg and candy>meat.
 - - Werewolf: Natural meat>veg and meat>candy.
 - - ...in other words, which food group is best represented.
 - - In a tie, you can satisfy any of the tied rules. So on Monday, you automatically win this.
 - +1 for a special rule, unique for each day. Just thinking out loud:
 - - Monday: All ingredients unique.
 - - Tuesday: All ingredients in pairs. "Twofer Tuesday!"
 - - Wednesday: No leftovers.
 - - Thursday: Leftovers at least double.
 - - Friday: No single-point ingredients.
 - - Saturday: Max 4 ingredients.
 - - Sunday: Must include secret sauce.
 - Reduce densities, put the max somewhere around 10 instead of 25.
 
Then each race has just one rule, to distinguish SAD from HAPPY.
These won't apply when Poison or Secret Sauce was used.
 - Man: Night clock below some threshold. Add a night clock!
 - Rabbit: Veg present.
 - Octopus: Candy present.
 - Werewolf: Meat present.
 - Princess: Always HAPPY.
 
Some consequences of this change:
 - Don't need large dining room graphics.
 - Daily deltas are uniform, I think that will feel more natural.
 - Per-day rule is a great touch for variety's sake.
 - Won't be much pressure to optimize for density.
 - The "no leftovers" rules might prove incompatible with secret sauces. Maybe sauces should never produce leftovers, their effective density clamps?
 - Every customer matters. When there's 200 of them, Apology Cards feel pointless. But just 20, yeah, you can bring them back retail.
 - ^ To press that harder: It wouldn't be crazy to draw 23 unique customers.
 - Dramatically reduces memory footprint, as if that matters.

This sounds good. Do it.
...early test runs: I love it. Much more interesting than before.

------------------------------------------

2025-02-10: Apportioned the 8 challenge zones.
With them all completely empty, I'm just barely able to reach every far corner in a single day.
Once there's some actual challenge involved, this should be good.
Two challenges per day should be possible, and 3 or 4 if you push it.
