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
#include <set>

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
NewsAggregator::NewsAggregator(const string &rssFeedListURI, bool verbose) : log(verbose), rssFeedListURI(rssFeedListURI), built(false) {}

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
	std::set<string> feedURLs;
	for (auto it : feeds)
	{
		string feedURL = it.first;
		string feedTitle = it.second;
		if (feedURLs.find(feedURL) != feedURLs.end())
		{
			log.noteSingleFeedDownloadSkipped(feedURL);
			continue;
		}
		RSSFeed feed(feedURL);
		try
		{
			log.noteSingleFeedDownloadBeginning(feedURL);
			feed.parse();
			log.noteSingleFeedDownloadEnd(feedURL);

			std::set<string> articleUrls;
			std::vector<Article> articles;
			for (Article article : feed.getArticles())
			{
				if (articleUrls.find(article.url) == articleUrls.end())
				{
					articleUrls.insert(article.url);
					articles.push_back(article);
				}
			}
			article2tokens(articles);
			log.noteAllArticlesHaveBeenScheduled(feedTitle);
		}
		catch (RSSFeedException &e)
		{
			log.noteSingleFeedDownloadFailure(feedURL);
			continue;
		}
	}
}

string dummyString = "-";
void NewsAggregator::article2tokens(std::vector<Article> articles)
{
	/**
	 * First sort articles with (title, url). This way, articles having same titles will neighbor each other.
	 * And since url's order is reversed, lexicographically smallest URL will be selected at last.
	 */
	sort(articles.begin(), articles.end(), [](Article &a, Article &b)
		 { return a.title != b.title ? a.title < b.title : a.url > b.url; });
	// for (size_t idx = 0; idx < articles.size(); idx++)
	// {
	// 	cout << articles[idx].title << " " << articles[idx].url << endl;
	// }
	// cout << endl;
	vector<string> tokensBefore;
	Article dummy;
	dummy.title = dummyString;
	dummy.url = dummyString;
	for (size_t idx = 0; idx < articles.size(); idx++)
	{
		Article article = articles[idx];
		log.noteSingleArticleDownloadBeginning(article);
		Article articleNext = idx < articles.size() - 1 ? articles[idx + 1] : dummy;
		HTMLDocument doc(article.url);
		try
		{
			doc.parse();
			vector<string> tokens = doc.getTokens();
			if (article.title == articleNext.title && getURLServer(article.url) == getURLServer(articleNext.url))
			{
				sort(tokens.begin(), tokens.end());
				if (tokensBefore.size() > 0)
				{
					vector<string> smallerList;
					set_intersection(tokensBefore.cbegin(), tokensBefore.cend(), tokens.cbegin(), tokens.cend(), back_inserter(smallerList));
					tokensBefore.clear();
					tokensBefore.insert(tokensBefore.begin(), smallerList.begin(), smallerList.end());
				}
				else
				{
					tokensBefore.insert(tokensBefore.begin(), tokens.begin(), tokens.end());
				}
			}
			else
			{
				if (tokensBefore.size())
				{
					sort(tokens.begin(), tokens.end());
					vector<string> smallerList;
					set_intersection(tokensBefore.cbegin(), tokensBefore.cend(), tokens.cbegin(), tokens.cend(), back_inserter(smallerList));
					index.add(article, smallerList);
					tokensBefore.clear();
				}
				else
				{
					index.add(article, tokens);
				}
			}
		}
		catch (HTMLDocumentException e)
		{
			log.noteSingleArticleDownloadFailure(article);
			continue;
		}
	}
}