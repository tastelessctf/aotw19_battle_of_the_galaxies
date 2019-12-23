#include <iostream>
#include <vector>
#include <algorithm>
#include <stdint.h>
#include <math.h>
// #include <sys/resource.h>
// #include <sys/time.h>

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
					stars[to].flights_to_enemy++;
				} else {
					add_link(links, from, to);
					stars[to].flights_to_friendly++;
				}
			} else if (cmd == "done") {
				break;
			}
		}

		// Call getrusage to query how much CPU time you have left.
		// Be a bit on the conservative side, since the moment the process exceeds
		// this limit, it is killed and your AI does nothing for the rest of the
		// match.
		// struct rusage usage;
		// getrusage(RUSAGE_SELF, &usage);
		// int usec_remaining =
		//     2000000 - ((usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000000 +
		//                usage.ru_utime.tv_usec + usage.ru_stime.tv_usec);

		for (auto & my : my_stars) {
			// cerr << " - "<< my->x << ", " << my->y << endl;
			if (my->ships > 11 && my->flights_from < 2) {
				// cerr << " ships " << my->ships << " " << free_stars.empty() << endl;
				for (auto & f : free_stars) {
					if (f->flights_to_friendly == 0 && f->flights_to_enemy == 0) {
						fly_to(*my, *f, 6);
					}
				}
			}

			if (my->ships > 30 && !enemy_stars.empty()) {
				auto target = enemy_stars[my->ships % enemy_stars.size()];
				fly_to(*my, *target, my->ships / 2);
			}

			if (my->ships > 20) {
				for (auto & other : our_stars) {
					if (!has_link(links, *my, *other) && my->ships >= other->ships) {
						fly_to(*my, *other, 1);
					}
				}
			}
		}
		cout << "done" << endl;
	}
}
