#include <iostream>
#include <utility>
#include "path.h"
#include <unordered_map>
#include <map>
#include <queue>
#include "imdb.h"
using namespace std;

static const int kWrongArgumentCount = 1;
static const int kDatabaseNotFound = 2;

bool bfs(const imdb &db, const string &source, const string &target, unordered_map<string, film> &before_actor, map<film, string> &before_film) {
	queue<string> q;
	q.push(source);
	film dummy {"dummy", 1900};
	before_actor[source] = dummy;
	int level = 7;

	while (q.size() > 0 && level-- > 0) {
		size_t length = q.size();
		for (size_t i = 0; i < length; i++) {
			string actor = q.front();
			q.pop();
			if (actor == target) {
				return true;
			}
			vector<film> movies;
			db.getCredits(actor, movies);
			for (auto movie : movies) {
				if (before_film.find(movie) == before_film.end()) {
					before_film[movie] = actor;
					vector<string> actors;
					db.getCast(movie, actors);
					for (auto next_actor : actors) {
						if (before_actor.find(next_actor) == before_actor.end()) {
							before_actor[next_actor] = movie;
							q.push(next_actor);
						}
					}
				}
			}
		}
	}
	return false;
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		cerr << "Usage: " << argv[0] << " <source-actor> <target-actor>" << endl;
		return kWrongArgumentCount;
	}
	imdb db(kIMDBDataDirectory);
	if (!db.good()) {
		cerr << "Data directory not found!  Aborting..." << endl; 
		return kDatabaseNotFound;
	}
	string source = argv[1];
	string target = argv[2];

	if (source == target) {
		cerr << "Maybe try different two people." << endl;
	}

	unordered_map<string, film> before_actor;
	map<film, string> before_film;

	bool flag = bfs(db, source, target, before_actor, before_film);
	
	if (flag) {
		path p(target);
		while (target != source) {
			film movie = before_actor[target];
			target = before_film[movie];
			p.addConnection(movie, target);
		}
		p.reverse();
		cout << p << endl;
	} else {
		cout << "No path between those two people could be found." << endl;
	}


	return 0;
}

