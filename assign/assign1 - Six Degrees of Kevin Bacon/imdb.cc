#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"

#include <string.h>
#include <algorithm>
#include <iterator>

#include <stdio.h>
using namespace std;

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";
imdb::imdb(const string& directory) {
	const string actorFileName = directory + "/" + kActorFileName;
	const string movieFileName = directory + "/" + kMovieFileName;  
	actorFile = acquireFileMap(actorFileName, actorInfo);
	movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const {
	return !( (actorInfo.fd == -1) || 
			(movieInfo.fd == -1) ); 
}

imdb::~imdb() {
	releaseFileMap(actorInfo);
	releaseFileMap(movieInfo);
}

// template<class ForwardIt, class T, class Compare=std::less<> >
// forwardIt binary_find(ForwardIt first, ForwardIt last, const T& value, Compare comp={})
// {
// Note: BOTH type T and the type after ForwardIt is dereferenced 
// must be implicitly convertible to BOTH Type1 and Type2, used in Compare. 
// This is stricter than lower_bound requirement (see above)

bool imdb::getCredits(const string& player, vector<film>& films) const {
	int count = *(int *) actorFile;
	int *start = ((int *) actorFile) + 1;
	int *offset = lower_bound(start, start + count, player, [this](const int offset, string player) -> bool {
									string targ = ((char *)actorFile) + offset;
									return targ < player;
									});
	char *info_pointer = ((char*)actorFile) + (*offset);
	string actor = info_pointer;
	if (actor == player) {
		size_t size = player.size() + 1;
		if ((size & 1) == 1){ // even
			size += 1;
		}
		info_pointer += size;
		short films_count = *(short *)info_pointer;
		info_pointer += 2;
		if ((size + 2) % 4 > 0) {
			info_pointer += 2;
		}

		while (films_count > 0) {
			int film_offset = *(int *)info_pointer;
			char *movie_pointer = (char *)movieFile + film_offset;
			film tmp;
			tmp.title = movie_pointer;
			movie_pointer += tmp.title.size() + 1;
			tmp.year = 1900 + *(unsigned char *)movie_pointer;
			films.push_back(tmp);
			films_count--;
			info_pointer += 4;
		}
		return true;
	}
	return false;
}

bool imdb::getCast(const film& movie, vector<string>& players) const { 	
	int count = *(int *) movieFile;
	int *start = ((int *) movieFile) + 1;
	int *offset = lower_bound(start, start + count, movie, [this](const int offset, film movie) -> bool {
									char *targ_pointer = ((char *)movieFile) + offset;
									string name = targ_pointer;
									if (movie.title == name) {
										targ_pointer += name.size() + 1;
										return 1900 + *(unsigned char *)targ_pointer < movie.year;
									}
									return name < movie.title;
									});
	char *info_pointer = ((char*)movieFile) + (*offset);
	string name = info_pointer;
	
	if (name == movie.title) {
		size_t size = name.size() + 1;
		info_pointer += size;
		// int year = 1900 + *(unsigned char *)info_pointer;
		info_pointer++; size++;
		if (size & 1) {
			info_pointer++; size++;
		}

		short actors_count = *(short *)info_pointer;
		info_pointer += 2;
		if ((size + 2) % 4 > 0) {
			info_pointer += 2;
		}

		while (actors_count > 0) {
			int actor_offset = *(int *)info_pointer;
			char *actor_pointer = (char *)actorFile + actor_offset;
			players.push_back(actor_pointer);
			actors_count--;
			info_pointer += 4;
		}
		return true;
	}
	return false;
}

const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info) {
	struct stat stats;
	stat(fileName.c_str(), &stats);
	info.fileSize = stats.st_size;
	info.fd = open(fileName.c_str(), O_RDONLY);
	return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info) {
	if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
	if (info.fd != -1) close(info.fd);
}
