#include <iostream>
#include <vector>
#include <algorithm>
#include <stdint.h>
#include <math.h>

using namespace std;

struct star {
	int id;
	int x;
	int y;
	int richness;
	int ships;
	int owner;
	int next_production;

	int flights_from;
	int flights_to_friendly;
	int flights_to_enemy;
	int attack_in;
	double score;
};

std::vector<star> stars;
int turn;

int distance(star & a, star & b)
{
	int dx = a.x - b.x;
	int dy = a.y - b.y;

	return ceil(sqrt(dx * dx + dy * dy));
}

#define MAP_SIZE 300
#define TRAVEL_SPEED 10

int travel_turns(star & from, star & to) {
    long squareDist = (long) (from.x - to.x) * (from.x - to.x) +
        (long) (from.y - to.y) * (from.y - to.y);

    int lb = 1;
    int ub = MAP_SIZE * 2;
    while (lb < ub) {
        int mb = (lb + ub) / 2;
        if (mb * mb >= squareDist) {
            ub = mb;
        } else {
            lb = mb + 1;
        }
    }
    return (lb + TRAVEL_SPEED - 1) / TRAVEL_SPEED;
}



void fly_to(star & from, star & to, int ships)
{
	if (distance(from, to) > 60) {
		return;
	}
	cout << "fly " << from.id << " " << to.id << " " << ships << endl;
	from.flights_from++;
	from.ships -= ships;
	to.flights_to_friendly++;
}

bool has_link(const std::vector<std::pair<int,int>> & links, star & a, star & b)
{
	int aa = std::min(a.id, b.id);
	int bb = std::max(a.id, b.id);
	return std::find(links.begin(), links.end(), std::make_pair(aa,bb)) != links.end();
}

void add_link(std::vector<std::pair<int,int>> & links, int a, int b)
{
	int aa = std::min(a, b);
	int bb = std::max(a, b);
	links.push_back({aa,bb});
}

std::vector<star *> nearby_stars(star & from, std::vector<star> & stars, int radius)
{
	std::vector<star *> nearby;
	for (auto & other : stars) {
		if (from.id != other.id) {
			if (distance(from, other) < radius) {
				nearby.push_back(&other);
			}
		}
	}
	return nearby;
}

void update_scores(std::vector<star> & stars)
{
	std::vector<star*> to_score;

	for (auto & star : stars) {
		if (star.owner < 0 || star.owner > 1) continue;
		star.score = 999;
		for (auto & enemy : stars) {
			if (enemy.owner == 2) {
				int dist = distance(star, enemy);
				if (dist < star.score) {
					star.score = dist;
				}
			}
		}
	}
}

void try_colonize(star & from, std::vector<star *> & candidates)
{
	std::sort(candidates.begin(), candidates.end(), [&from](star * a, star * b) {
		if (a->richness > b->richness) {
			return true;
		} else if (a->richness == b->richness) {
			int dist_a = distance(from, *a);
			int dist_b = distance(from, *b);
			return dist_a < dist_b;
		} else {
			return false;
		}
		});

	for (auto c : candidates) {

		if (from.ships > 5 &&
			c->flights_to_friendly == 0 && c->flights_to_enemy == 0)
		{
			fly_to(from, *c, 6);
            continue;
		}
        // let's see if we can team up with other planet
	    auto candidate_neighbours = nearby_stars(*c, stars, 60);
        for (auto helper : candidate_neighbours) {
            if(c->flights_to_friendly != 0 || c->flights_to_enemy != 0) continue; //theres work in progress, let's not get involved
            if (helper->id == from.id) continue; // we cant help ourselves
            if (helper->owner != from.owner) continue; //this is not our star
            if (helper->ships + from.ships < 6) continue; // are we strong enough
            if ( travel_turns(*helper, *c) != travel_turns(from, *c)) continue; //we don't arrive at the same time
            int missing_ships = 6 - from.ships;
            fly_to(from, *c, from.ships);
            fly_to(*helper, *c, missing_ships);

        }
	}
}

void link_with_others(const std::vector<std::pair<int, int>> & links, star & from, std::vector<star *> & candidates)
{

        auto nearby = nearby_stars(from, stars, 60);
		for (auto & c: candidates) {
            if (c->owner == -1) return; // dont link
        }

		for (auto & other : candidates) {
            if (from.ships > 0 && other->flights_to_friendly == 0 ) {
			if (!has_link(links, from, *other) && from.ships >= other->ships) {
				fly_to(from, *other, 1);
			}
		}
	}
}

void attack(star & from, std::vector<star *> & enemies)
{
	std::sort(enemies.begin(), enemies.end(), [](star * a, star * b) {
		if (a->richness == b->richness) {
			return a->ships < b->ships;
		} else {
			return a->richness > b->richness;
		}
		});

	for (auto target: enemies) {
		int strength = 10 + target->ships + target->richness + target->flights_to_enemy;
		int to_send = from.ships - 10 - from.flights_to_enemy;
		if (to_send > strength) {
			to_send = std::min(to_send, strength * 2);	// send max twice as many ships as needed
			fly_to(from, *target, to_send);
		}
	}
}

bool is_friendly(star & who) {
    if (who.id == 1 || who.id == 0) return true;
    return false;
}

bool safe_neighbourhood(star & source, int depth)
{
    if (depth == 0) return true;
    auto nearby = nearby_stars(source, stars, 40); //only intel distance
    for (auto neighbour : nearby) {
		if (!is_friendly( *neighbour )) break;
        return safe_neighbourhood(*neighbour, depth-1);
    }
    return false;
}

void to_front_lines(star & from, std::vector<star *> & friendlies)
{
	if (from.score < 60) {
		// we are at front line
		return;
	}

	int safe_ships = (60 - from.score) / 2;
	if (safe_ships < 0) {
		safe_ships = 0;
	}

	if (from.ships < safe_ships) {
		// we don't have enough ships
		return;
	}

	std::sort(friendlies.begin(), friendlies.end(), [](star * a, star * b) {
		return a->score < b->score;
		});

	for (auto target: friendlies) {
		if (target->score < from.score) {
			fly_to(from, *target, from.ships - safe_ships);
			break;
		}
	}
}

void can_help(star & from, std::vector<star *> & friends)
{
	if (from.attack_in) {
		// sorry, can't help as we are under attack too
		return;
	}
	for (auto f : friends) {
		if (f->attack_in) {
			// under attack, can we help?
			int dist = distance(from, *f);
			int turns = dist / 10;
			if (turns < f->attack_in) {
				// we are in range
				int reinforcements_needed = f->ships - (f->flights_to_enemy - 10) + 1;
				if (reinforcements_needed > 0 && from.ships - reinforcements_needed > 10) {
					// looks bearable, send help
					fly_to(from, *f, reinforcements_needed);
				}
			}
		}
	}
}



int main() {
	// Disable synchronization to make cin/cout much faster.
	// Don't use scanf/printf then. Of course, you can do your
	// own scanf without using cin/cout if you decide to.
	// This is just an example.
	std::ios::sync_with_stdio(false);

	// Read initial galaxy configuration.
	string dummy;
	cin >> dummy;
	for (int i = 0; i < 90; i++) {
		star s = {i};
		cin >> s.x >> s.y;
		s.richness = 1;
		s.owner = -1;
		stars.push_back(s);
	}

	while (true) {
		std::vector<star *> free_stars;
		std::vector<star *> my_stars;
		std::vector<star *> our_stars;
		std::vector<star *> enemy_stars;
		std::vector<std::pair<int,int>> links;

		// cerr << "================= TURN " << turn << endl;

		for (auto & star : stars) {
			star.flights_from = 0;
			star.flights_to_friendly = 0;
			star.flights_to_enemy = 0;
			star.attack_in = 0;
		}

		while (true) {
			if (!cin)
				return 0;
			string cmd;
			cin >> cmd;
			if (cmd == "star") {
				int id, richness, owner, shipcount, turns;
				cin >> id >> richness >> owner >> shipcount >> turns;

				star & intel = stars[id];
				intel.richness = richness;
				intel.owner = owner;
				intel.ships = shipcount;
				intel.next_production = turns;
				// cerr << "intel " <<  " - " << intel.x << " " << intel.y << " " << intel.owner << " " << intel.ships << endl;
				switch (owner) {
					case -1:
						free_stars.push_back(&intel);
						break;
					case 0:
						my_stars.push_back(&intel);
						break;
					case 1:
						our_stars.push_back(&intel);
						break;
					case 2:
						enemy_stars.push_back(&intel);
						break;
				}
			} else if (cmd == "link") {
				int from, to;
				cin >> from >> to;
				links.push_back({from,to});
			} else if (cmd == "flight") {
				int from, to, owner, shipcount, turns;
				cin >> from >> to >> shipcount >> owner >> turns;

				stars[from].flights_from++;
				if (owner == 2) {
					stars[to].flights_to_enemy += shipcount;
					if (stars[to].attack_in == 0 || turns < stars[to].attack_in) {
						stars[to].attack_in = turns;
					}
				} else {
					add_link(links, from, to);
					stars[to].flights_to_friendly++;
				}
			} else if (cmd == "done") {
				break;
			}
		}

		update_scores(stars);


		for (auto & my : my_stars) {
			auto nearby = nearby_stars(*my, stars, 60);
			std::vector<star *> nearby_enemies;
			std::vector<star *> nearby_friends;

			std::copy_if(nearby.begin(), nearby.end(), std::back_inserter(nearby_enemies), [](star * s) {
				return s->owner == 2;
				});

			std::copy_if(nearby.begin(), nearby.end(), std::back_inserter(nearby_friends), [](star * s) {
				return s->owner == 0 || s->owner == 1;
				});

			try_colonize(*my, free_stars);
			can_help(*my, nearby_friends);
			link_with_others(links, *my, our_stars);
			attack(*my, nearby_enemies);
			to_front_lines(*my, nearby_friends);
		}

        /* nsr: move all leftover troops based on adj matrix by simple heuristic:
         * a) check if all neighbour nodes are friendly
         * b) move troops to neighbour node with most edges
        for (auto & my : my_stars) {
            if (safe_neighbourhood(*my, 1)) {
                auto nearby = nearby_stars(*my, stars, 60);

                  std::sort(nearby.begin(), nearby.end(),
                        [](star * a, star * b) {
                    return a->score > b->score;
                });

                fly_to( *my, *nearby.front(), my->ships-10);
            }
        }
         */


		cout << "done" << endl;
		turn++;
	}
} 
