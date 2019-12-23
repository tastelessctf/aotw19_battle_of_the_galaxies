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
	double score;
};

std::vector<star> stars;

int distance(star & a, star & b)
{
	int dx = a.x - b.x;
	int dy = a.y - b.y;

	return floor(sqrt(dx * dx + dy * dy));
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

std::vector<star *> nearby_stars(star & from, std::vector<star> & stars)
{
	std::vector<star *> nearby;
	for (auto & other : stars) {
		if (from.id != other.id) {
			if (distance(from, other) < 60) {
				nearby.push_back(&other);
			}
		}
	}
	return nearby;
}

int score_multiplier(star & s)
{
	if (s.owner == -1) return 0;
	if (s.owner == 2) return -1;
	return 1;
}

void update_scores(std::vector<star> & stars)
{
	std::vector<star*> to_score;

	for (auto & star : stars) {
		if (star.owner < 0 || star.owner > 1) continue;
		star.score = 1000;
		auto nearby = nearby_stars(star, stars);
		for (auto near : nearby) {
			if (near->owner == 2) {
				int dist = distance(star, *near);
				if (dist < star.score) {
					star.score = dist;
				}
			}
		}
		if (star.score > 999) {
			// no enemy in vicinity, need further computation
			to_score.push_back(&star);
		}
	}
	int rounds = 0;
	while (!to_score.empty() || rounds++ > 7) {
		for (auto s : to_score) {
			auto nearby = nearby_stars(*s, stars);
			for (auto near : nearby) {
				if (near->score < s->score) {
					int dist = distance(*s, *near);
					if (dist + near->score < s->score) {
						s->score = dist + near->score;
					}
				}
			}
		}
		to_score.erase(std::remove_if(to_score.begin(), to_score.end(), [](star * a){ return a->score < 1000;}), to_score.end());
	}
}

void try_colonize(star & from, std::vector<star *> & candidates)
{
	std::sort(candidates.begin(), candidates.end(), [](star * a, star * b) {return a->richness > b->richness;});

	for (auto c : candidates) {
		if (from.ships > 5 &&
			c->flights_to_friendly == 0 && c->flights_to_enemy == 0)
		{
			fly_to(from, *c, 6);
		}
	}
}

void link_with_others(const std::vector<std::pair<int, int>> & links, star & from, std::vector<star *> & candidates)
{
	if (from.ships > 20) {
		for (auto & other : candidates) {
			if (!has_link(links, from, *other) && from.ships >= other->ships) {
				fly_to(from, *other, 1);
			}
		}
	}
}

void send_ships(star & from, std::vector<star> & stars)
{
	std::vector<star *> nearby = nearby_stars(from, stars);

	std::sort(nearby.begin(), nearby.end(), [](star * a, star * b) {
        if (a->owner < 2 && b->owner < 2) {
			return a->score < b->score;
		} else if (a->owner == b->owner) {
			return a->ships > b->ships;
		} else {
			return a->owner == 2;
		}
		});

	for (auto target: nearby) {
		if (target->owner == 2) {
			if (from.ships - 20 > target->ships + target->richness + target->flights_to_enemy) {
				fly_to(from, *target, from.ships - 10);
			}
		} else {
			if (target->score < from.score && from.ships > 30) {
				fly_to(from, *target, from.ships - 20);
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

	int turn = 0;
	while (true) {
		std::vector<star *> free_stars;
		std::vector<star *> my_stars;
		std::vector<star *> our_stars;
		std::vector<star *> enemy_stars;
		std::vector<std::pair<int,int>> links;

//		cerr << "================= TURN " << turn << endl;

		for (auto & star : stars) {
			star.flights_from = 0;
			star.flights_to_friendly = 0;
			star.flights_to_enemy = 0;
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
			try_colonize(*my, free_stars);
			link_with_others(links, *my, our_stars);
			send_ships(*my, stars);
		}

		cout << "done" << endl;
		turn++;
	}
}
