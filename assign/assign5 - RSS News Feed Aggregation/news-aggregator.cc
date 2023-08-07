/**
 * File: news-aggregator.cc
 * --------------------------------
 * Presents the implementation of the NewsAggregator class.
 */

#include "news-aggregator.h"
#include <iostream>
#include <iomanip>
#include <getopt.h>
#include <libxml/parser.h>
#include <libxml/catalog.h>
// you will almost certainly need to add more system header includes
#include <thread>

// I'm not giving away too much detail here by leaking the #includes below,
// which contribute to the official CS110 staff solution.
#include "rss-feed.h"
#include "rss-feed-list.h"
#include "html-document.h"
#include "html-document-exception.h"
#include "rss-feed-exception.h"
#include "rss-feed-list-exception.h"
#include "utils.h"
#include "ostreamlock.h"
#include "string-utils.h"
using namespace std;

/**
 * Factory Method: createNewsAggregator
 * ------------------------------------
 * Factory method that spends most of its energy parsing the argument vector
 * to decide what rss feed list to process and whether to print lots of
 * of logging information as it does so.
 */
static const string kDefaultRSSFeedListURL = "small-feed.xml";
NewsAggregator *NewsAggregator::createNewsAggregator(int argc, char *argv[])
{
	struct option options[] = {
		{"verbose", no_argument, NULL, 'v'},
		{"quiet", no_argument, NULL, 'q'},
		{"url", required_argument, NULL, 'u'},
		{NULL, 0, NULL, 0},
	};

	string rssFeedListURI = kDefaultRSSFeedListURL;
	bool verbose = false;
	while (true)
	{
		int ch = getopt_long(argc, argv, "vqu:", options, NULL);
		if (ch == -1)
			break;
		switch (ch)
		{
		case 'v':
			verbose = true;
			break;
		case 'q':
			verbose = false;
			break;
		case 'u':
			rssFeedListURI = optarg;
			break;
		default:
			NewsAggregatorLog::printUsage("Unrecognized flag.", argv[0]);
		}
	}

	argc -= optind;
	if (argc > 0)
		NewsAggregatorLog::printUsage("Too many arguments.", argv[0]);
	return new NewsAggregator(rssFeedListURI, verbose);
}

/**
 * Method: buildIndex
 * ------------------
 * Initalizex the XML parser, processes all feeds, and then
 * cleans up the parser.  The lion's share of the work is passed
 * on to processAllFeeds, which you will need to implement.
 */
void NewsAggregator::buildIndex()
{
	if (built)
		return;
	built = true; // optimistically assume it'll all work out
	xmlInitParser();
	xmlInitializeCatalog();
	processAllFeeds();
	xmlCatalogCleanup();
	xmlCleanupParser();
}

/**
 * Method: queryIndex
 * ------------------
 * Interacts with the user via a custom command line, allowing
 * the user to surface all of the news articles that contains a particular
 * search term.
 */
void NewsAggregator::queryIndex() const
{
	static const size_t kMaxMatchesToShow = 15;
	while (true)
	{
		cout << "Enter a search term [or just hit <enter> to quit]: ";
		string response;
		getline(cin, response);
		response = trim(response);
		if (response.empty())
			break;
		const vector<pair<Article, int>> &matches = index.getMatchingArticles(response);
		if (matches.empty())
		{
			cout << "Ah, we didn't find the term \"" << response << "\". Try again." << endl;
		}
		else
		{
			cout << "That term appears in " << matches.size() << " article"
				 << (matches.size() == 1 ? "" : "s") << ".  ";
			if (matches.size() > kMaxMatchesToShow)
				cout << "Here are the top " << kMaxMatchesToShow << " of them:" << endl;
			else if (matches.size() > 1)
				cout << "Here they are:" << endl;
			else
				cout << "Here it is:" << endl;
			size_t count = 0;
			for (const pair<Article, int> &match : matches)
			{
				if (count == kMaxMatchesToShow)
					break;
				count++;
				string title = match.first.title;
				if (shouldTruncate(title))
					title = truncate(title);
				string url = match.first.url;
				if (shouldTruncate(url))
					url = truncate(url);
				string times = match.second == 1 ? "time" : "times";
				cout << "  " << setw(2) << setfill(' ') << count << ".) "
					 << "\"" << title << "\" [appears " << match.second << " " << times << "]." << endl;
				cout << "       \"" << url << "\"" << endl;
			}
		}
	}
}

/**
 * Private Constructor: NewsAggregator
 * -----------------------------------
 * Self-explanatory.  You may need to add a few lines of code to
 * initialize any additional fields you add to the private section
 * of the class definition.
 */
NewsAggregator::NewsAggregator(const string &rssFeedListURI, bool verbose) : log(verbose), rssFeedListURI(rssFeedListURI), built(false), grandChildPermits(grandchildMaxNum) {}

/**
 * Private Method: processAllFeeds
 * -------------------------------
 * Downloads and parses the encapsulated RSSFeedList, which itself
 * leads to RSSFeeds, which themsleves lead to HTMLDocuemnts, which
 * can be collectively parsed for their tokens to build a huge RSSIndex.
 *
 * The vast majority of your Assignment 5 work has you implement this
 * method using multithreading while respecting the imposed constraints
 * outlined in the spec.
 */
void NewsAggregator::processAllFeeds()
{
	RSSFeedList feedList(rssFeedListURI.c_str());
	try
	{
		feedList.parse();
	}
	catch (RSSFeedListException e)
	{
		log.noteFullRSSFeedListDownloadFailureAndExit(rssFeedListURI);
		return;
	}
	log.noteFullRSSFeedListDownloadEnd();
	feed2articles(feedList.getFeeds());
	log.noteAllRSSFeedsDownloadEnd();
	return;
}

void NewsAggregator::feed2articles(std::map<std::string, std::string> feeds)
{
	vector<thread> threads;
	semaphore permits(childMaxNum);
	for (auto it : feeds)
	{
		permits.wait();
		threads.emplace_back(thread([this, it](semaphore &s)
									{
			s.signal(on_thread_exit);
			string feedURL = it.first;
			string feedTitle = it.second;

			feedURLsLock.lock();
			if (feedURLs.find(feedURL) != feedURLs.end())
			{
				log.noteSingleFeedDownloadSkipped(feedURL);
				feedURLsLock.unlock();
				return;
			}
			feedURLs.insert(feedURL);
			feedURLsLock.unlock();

			RSSFeed feed(feedURL);
			try
			{
				log.noteSingleFeedDownloadBeginning(feedURL);
				feed.parse();
				log.noteSingleFeedDownloadEnd(feedURL);

				article2tokens(feed.getArticles());
				log.noteAllArticlesHaveBeenScheduled(feedTitle);
			}
			catch (RSSFeedException &e)
			{
				log.noteSingleFeedDownloadFailure(feedURL);
				return;
			} },
									ref(permits)));
	}
	for (thread &t : threads)
	{
		t.join();
	}
}

void NewsAggregator::article2tokens(std::vector<Article> articles)
{
	/**
	 * 'result' is..
	 * map<{title, urlServer}, {article, tokens}>
	 */
	map<pair<string, string>, pair<Article, vector<string>>> result;
	mutex resultLock;
	vector<thread> threads;
	for (Article article : articles)
	{
		grandChildPermits.wait();
		threads.push_back(thread([this, article](map<pair<string, string>, pair<Article, vector<string>>> &result, mutex &resultLock)
								 {
			// grandChild thread max 18						
			grandChildPermits.signal(on_thread_exit);

			// download in the same server max 8
			string urlServer = getURLServer(article.url);
			serverConnectionPermitsLock.lock();
			if (!serverConnectionPermits.count(urlServer)) {
				serverConnectionPermits[urlServer] = new semaphore(serverConnectionMaxNum);
			}
			serverConnectionPermits[urlServer]->wait();
			serverConnectionPermits[urlServer]->signal(on_thread_exit);
			serverConnectionPermitsLock.unlock();

			// skip duplicated url
			articleURLsLock.lock();
			if (articleURLs.find(article.url) != articleURLs.end())
			{
				log.noteSingleArticleDownloadSkipped(article);
				articleURLsLock.unlock();
				return;
			}
			articleURLs.insert(article.url);
			articleURLsLock.unlock();

			// setup downloading
			pair<string, string> theKey{article.title, getURLServer(article.url)};
			HTMLDocument doc(article.url);
			// start downloading
			try
			{
				log.noteSingleArticleDownloadBeginning(article);
				doc.parse();
			}
			catch (HTMLDocumentException e)
			{
				log.noteSingleArticleDownloadFailure(article);
				return;
			} 
			vector<string> tokens = doc.getTokens();
			sort(tokens.begin(), tokens.end());

			resultLock.lock();
			if (result.count(theKey) > 0)
			{
				vector<string> smallerList;
				set_intersection(result[theKey].second.cbegin(), result[theKey].second.cend(),
								tokens.cbegin(), tokens.cend(), back_inserter(smallerList));
				result[theKey].second = smallerList;
				if (article.url < result[theKey].first.url)
				{
					result[theKey].first = article;
				}
			}
			else
			{
				result[theKey] = {article, tokens};
			}
			resultLock.unlock(); },
								 ref(result), ref(resultLock)));
	}
	for (thread &t : threads)
	{
		t.join();
	}
	// sequential adding to the index. But since this function is multi threaded, need lock.
	for (auto it : result)
	{
		indexLock.lock();
		index.add(it.second.first, it.second.second);
		indexLock.unlock();
	}
}