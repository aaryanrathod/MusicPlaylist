#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LEN 100
#define MAX_PLAYLISTS 50

// Song node (Doubly Linked List)
typedef struct Song
{
    int id;
    char title[MAX_LEN];
    char artist[MAX_LEN];
    struct Song *prev, *next;
} Song;

// Playlist
typedef struct
{
    char name[MAX_LEN];
    Song *head, *tail, *current;
    int size;
} Playlist;

// History node (Stack using Singly Linked List)
typedef struct HistoryNode
{
    int id;
    char title[MAX_LEN];
    char artist[MAX_LEN];
    struct HistoryNode *next;
} HistoryNode;

// Globals
Playlist *playlists[MAX_PLAYLISTS];
int playlistCount = 0;
Playlist *active = NULL;
int activeIndex = 0;
HistoryNode *historyTop = NULL;
int historyCount = 0;
int repeatMode = 0;

// --- Utility ---

void readString(char *str, int maxLen)
{
    fgets(str, maxLen, stdin);
    str[strcspn(str, "\n")] = '\0';
}

char toLowerChar(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c + 32;
    return c;
}

int cmpIgnoreCase(const char *a, const char *b)
{
    while (*a && *b)
    {
        if (toLowerChar(*a) != toLowerChar(*b))
            return toLowerChar(*a) - toLowerChar(*b);
        a++;
        b++;
    }
    return *a - *b;
}

int containsSubstr(const char *str, const char *sub)
{
    int i, j, slen = strlen(str), sublen = strlen(sub);
    int found = 0;
    if (sublen == 0)
        return 1;
    i = 0;
    while (i <= slen - sublen && !found)
    {
        int match = 1;
        j = 0;
        while (j < sublen && match)
        {
            if (toLowerChar(str[i + j]) != toLowerChar(sub[j]))
                match = 0;
            else
                j++;
        }
        if (match)
            found = 1;
        else
            i++;
    }
    return found;
}

// --- Song/Playlist Functions ---

Song *createSong(int id, char *title, char *artist)
{
    Song *s = (Song *)malloc(sizeof(Song));
    if (!s)
    {
        printf("Memory allocation failed!\n");
        return NULL;
    }
    s->id = id;
    strcpy(s->title, title);
    strcpy(s->artist, artist);
    s->prev = s->next = NULL;
    return s;
}

Song *findByID(Playlist *pl, int id)
{
    Song *temp = pl->head;
    while (temp)
    {
        if (temp->id == id)
            return temp;
        temp = temp->next;
    }
    return NULL;
}

// Insert song in sorted order by SongID. Returns 1=success, 0=duplicate, -1=error
int addSong(Playlist *pl, int id, char *title, char *artist)
{
    if (findByID(pl, id))
        return 0;

    Song *newSong = createSong(id, title, artist);
    if (!newSong)
        return -1;

    // Empty list
    if (!pl->head)
    {
        pl->head = pl->tail = pl->current = newSong;
        pl->size++;
        return 1;
    }

    // Before head
    if (id < pl->head->id)
    {
        newSong->next = pl->head;
        pl->head->prev = newSong;
        pl->head = newSong;
        pl->size++;
        return 1;
    }

    // After tail
    if (id > pl->tail->id)
    {
        newSong->prev = pl->tail;
        pl->tail->next = newSong;
        pl->tail = newSong;
        pl->size++;
        return 1;
    }

    // Middle
    Song *temp = pl->head;
    while (temp->next && temp->next->id < id)
        temp = temp->next;

    newSong->next = temp->next;
    newSong->prev = temp;
    if (temp->next)
        temp->next->prev = newSong;
    temp->next = newSong;
    pl->size++;
    return 1;
}

int deleteSong(Playlist *pl, int id)
{
    Song *song = findByID(pl, id);
    if (!song)
        return 0;

    // Update current pointer if deleting the playing song
    if (pl->current == song)
    {
        if (song->next)
            pl->current = song->next;
        else if (song->prev)
            pl->current = song->prev;
        else
            pl->current = NULL;
    }

    if (song == pl->head && song == pl->tail)
    {
        pl->head = pl->tail = NULL;
    }
    else if (song == pl->head)
    {
        pl->head = song->next;
        pl->head->prev = NULL;
    }
    else if (song == pl->tail)
    {
        pl->tail = song->prev;
        pl->tail->next = NULL;
    }
    else
    {
        song->prev->next = song->next;
        song->next->prev = song->prev;
    }

    printf("Deleted: [%d] %s - %s\n", song->id, song->title, song->artist);
    free(song);
    pl->size--;
    return 1;
}

void printSongRow(Song *s)
{
    printf("  %-6d | %-30s | %s\n", s->id, s->title, s->artist);
}

void printTableHeader()
{
    printf("  %-6s | %-30s | %s\n", "ID", "Title", "Artist");
    printf("  ------+--------------------------------+---------------------------\n");
}

void displayPlaylist(Playlist *pl)
{
    if (pl->size == 0)
    {
        printf("Playlist is empty.\n");
        return;
    }
    printf("\n");
    printTableHeader();
    Song *temp = pl->head;
    while (temp)
    {
        printSongRow(temp);
        temp = temp->next;
    }
    printf("  Total: %d song(s)\n\n", pl->size);
}

// Copy song pointers into an array for sorting
Song **toArray(Playlist *pl)
{
    Song **arr = (Song **)malloc(pl->size * sizeof(Song *));
    if (!arr)
        return NULL;
    Song *temp = pl->head;
    int i;
    for (i = 0; temp; i++)
    {
        arr[i] = temp;
        temp = temp->next;
    }
    return arr;
}

// Display sorted by: 1=Title, 2=Artist, 3=Artist then Title
void displaySorted(Playlist *pl, int mode)
{
    if (pl->size == 0)
    {
        printf("Playlist is empty.\n");
        return;
    }

    Song **arr = toArray(pl);
    if (!arr)
    {
        printf("Memory allocation failed!\n");
        return;
    }

    int i, j;
    // Bubble sort
    for (i = 0; i < pl->size - 1; i++)
    {
        for (j = 0; j < pl->size - 1 - i; j++)
        {
            int doSwap = 0;
            if (mode == 1)
            {
                doSwap = cmpIgnoreCase(arr[j]->title, arr[j + 1]->title) > 0;
            }
            else if (mode == 2)
            {
                doSwap = cmpIgnoreCase(arr[j]->artist, arr[j + 1]->artist) > 0;
            }
            else
            {
                int cmp = cmpIgnoreCase(arr[j]->artist, arr[j + 1]->artist);
                if (cmp > 0)
                    doSwap = 1;
                else if (cmp == 0)
                    doSwap = cmpIgnoreCase(arr[j]->title, arr[j + 1]->title) > 0;
            }
            if (doSwap)
            {
                Song *tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }

    const char *labels[] = {"", "Title", "Artist", "Artist, then Title"};
    printf("\n  (Sorted by %s)\n", labels[mode]);
    printTableHeader();
    for (i = 0; i < pl->size; i++)
        printSongRow(arr[i]);
    printf("  Total: %d song(s)\n\n", pl->size);
    free(arr);
}

// --- Search ---

void searchByID(Playlist *pl, int id)
{
    Song *s = findByID(pl, id);
    if (s)
    {
        printf("Song found:\n");
        printTableHeader();
        printSongRow(s);
    }
    else
    {
        printf("Song with ID %d not found.\n", id);
    }
}

void searchByTitle(Playlist *pl, char *query)
{
    int found = 0;
    Song *temp = pl->head;
    while (temp)
    {
        if (containsSubstr(temp->title, query))
        {
            if (!found)
            {
                printf("Search results:\n");
                printTableHeader();
                found = 1;
            }
            printSongRow(temp);
        }
        temp = temp->next;
    }
    if (!found)
        printf("No songs found matching title \"%s\".\n", query);
}

void searchByArtist(Playlist *pl, char *query)
{
    int found = 0;
    Song *temp = pl->head;
    while (temp)
    {
        if (containsSubstr(temp->artist, query))
        {
            if (!found)
            {
                printf("Search results:\n");
                printTableHeader();
                found = 1;
            }
            printSongRow(temp);
        }
        temp = temp->next;
    }
    if (!found)
        printf("No songs found by artist \"%s\".\n", query);
}

// --- Player Controls ---

void showNowPlaying(Playlist *pl)
{
    if (!pl->current)
    {
        printf("No song is currently playing.\n");
        return;
    }
    printf("\nNow Playing:\n");
    printf("  ID:     %d\n", pl->current->id);
    printf("  Title:  %s\n", pl->current->title);
    printf("  Artist: %s\n\n", pl->current->artist);
}

// --- History (Stack) ---

void addToHistory(int id, char *title, char *artist)
{
    HistoryNode *node = (HistoryNode *)malloc(sizeof(HistoryNode));
    if (!node)
    {
        printf("Memory allocation failed!\n");
        return;
    }
    node->id = id;
    strcpy(node->title, title);
    strcpy(node->artist, artist);
    node->next = historyTop;
    historyTop = node;
    historyCount++;
}

void playNext(Playlist *pl)
{
    if (!pl->current)
    {
        printf("Playlist is empty.\n");
        return;
    }
    if (pl->current->next)
    {
        pl->current = pl->current->next;
    }
    else if (repeatMode)
    {
        pl->current = pl->head;
        printf("Playlist looped to the beginning.\n");
    }
    else
    {
        printf("End of playlist. Turn on repeat mode to loop.\n");
        return;
    }
    addToHistory(pl->current->id, pl->current->title, pl->current->artist);
    showNowPlaying(pl);
}

void playPrev(Playlist *pl)
{
    if (!pl->current)
    {
        printf("Playlist is empty.\n");
        return;
    }
    if (pl->current->prev)
    {
        pl->current = pl->current->prev;
    }
    else if (repeatMode)
    {
        pl->current = pl->tail;
        printf("Playlist looped to the end.\n");
    }
    else
    {
        printf("Beginning of playlist. Turn on repeat mode to loop.\n");
        return;
    }
    addToHistory(pl->current->id, pl->current->title, pl->current->artist);
    showNowPlaying(pl);
}

void shufflePlaylist(Playlist *pl)
{
    if (pl->size < 2)
    {
        printf("Not enough songs to shuffle.\n");
        return;
    }

    Song **arr = toArray(pl);
    if (!arr)
        return;

    int i;
    for (i = pl->size - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        Song *tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }

    for (i = 0; i < pl->size; i++)
    {
        arr[i]->prev = (i > 0) ? arr[i - 1] : NULL;
        arr[i]->next = (i < pl->size - 1) ? arr[i + 1] : NULL;
    }
    pl->head = arr[0];
    pl->tail = arr[pl->size - 1];
    free(arr);
    printf("Playlist shuffled.\n");
}

void restoreSortedOrder(Playlist *pl)
{
    if (pl->size < 2)
    {
        printf("Not enough songs to reorder.\n");
        return;
    }

    Song **arr = toArray(pl);
    if (!arr)
        return;

    // Bubble sort by Song ID
    int i, j;
    for (i = 0; i < pl->size - 1; i++)
    {
        for (j = 0; j < pl->size - 1 - i; j++)
        {
            if (arr[j]->id > arr[j + 1]->id)
            {
                Song *tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }

    // Rebuild linked list from sorted array
    for (i = 0; i < pl->size; i++)
    {
        arr[i]->prev = (i > 0) ? arr[i - 1] : NULL;
        arr[i]->next = (i < pl->size - 1) ? arr[i + 1] : NULL;
    }
    pl->head = arr[0];
    pl->tail = arr[pl->size - 1];
    free(arr);
    printf("Playlist restored to sorted order (by Song ID).\n");
}

void repeatDisplay(Playlist *pl)
{
    if (pl->size == 0)
    {
        printf("Playlist is empty.\n");
        return;
    }
    int loop;
    printf("Displaying playlist on repeat (3 loops):\n");
    for (loop = 1; loop <= 3; loop++)
    {
        printf("\n--- Loop %d ---\n", loop);
        Song *temp = pl->head;
        int num = 1;
        while (temp)
        {
            printf("  %d. [%d] %s - %s\n", num++, temp->id, temp->title, temp->artist);
            temp = temp->next;
        }
    }
    printf("\n");
}

// Chronological (oldest first) using recursion
void printChrono(HistoryNode *node, int *num)
{
    if (!node)
        return;
    printChrono(node->next, num);
    printf("  %-4d | %-6d | %-30s | %s\n", (*num)++, node->id, node->title, node->artist);
}

void displayHistoryChrono()
{
    if (!historyTop)
    {
        printf("Play history is empty.\n");
        return;
    }
    printf("\nPlay History (Oldest First):\n");
    printf("  %-4s | %-6s | %-30s | %s\n", "#", "ID", "Title", "Artist");
    printf("  -----+--------+--------------------------------+---------------------------\n");
    int num = 1;
    printChrono(historyTop, &num);
    printf("  Total: %d song(s) played.\n\n", historyCount);
}

void displayHistoryReverse()
{
    if (!historyTop)
    {
        printf("Play history is empty.\n");
        return;
    }
    printf("\nPlay History (Most Recent First):\n");
    printf("  %-4s | %-6s | %-30s | %s\n", "#", "ID", "Title", "Artist");
    printf("  -----+--------+--------------------------------+---------------------------\n");
    HistoryNode *temp = historyTop;
    int num = 1;
    while (temp)
    {
        printf("  %-4d | %-6d | %-30s | %s\n", num++, temp->id, temp->title, temp->artist);
        temp = temp->next;
    }
    printf("  Total: %d song(s) played.\n\n", historyCount);
}

// --- Multi-Playlist Operations ---

void freePlaylist(Playlist *pl)
{
    Song *temp = pl->head;
    while (temp)
    {
        Song *next = temp->next;
        free(temp);
        temp = next;
    }
    pl->head = pl->tail = pl->current = NULL;
    pl->size = 0;
}

Playlist *createEmptyPlaylist()
{
    Playlist *pl = (Playlist *)malloc(sizeof(Playlist));
    if (!pl)
    {
        printf("Memory allocation failed!\n");
        return NULL;
    }
    pl->head = pl->tail = pl->current = NULL;
    pl->size = 0;
    pl->name[0] = '\0';
    return pl;
}

Playlist *createNamedPlaylist(const char *name)
{
    Playlist *pl = createEmptyPlaylist();
    if (pl)
        strcpy(pl->name, name);
    return pl;
}

int addPlaylistToSystem(Playlist *pl)
{
    if (playlistCount >= MAX_PLAYLISTS)
    {
        printf("Maximum number of playlists reached!\n");
        return 0;
    }
    playlists[playlistCount] = pl;
    playlistCount++;
    return 1;
}

void listAllPlaylists()
{
    int i;
    printf("\n--- All Playlists ---\n");
    for (i = 0; i < playlistCount; i++)
    {
        printf("  %d. %s (%d songs)%s\n", i + 1, playlists[i]->name,
               playlists[i]->size, (playlists[i] == active) ? "  <-- ACTIVE" : "");
    }
    printf("\n");
}

// Select a playlist by number (returns index, or -1 on failure)
int selectPlaylist(const char *prompt)
{
    int sel;
    listAllPlaylists();
    printf("%s", prompt);
    if (scanf("%d", &sel) != 1 || sel < 1 || sel > playlistCount)
    {
        printf("Invalid selection!\n");
        scanf("%*[^\n]");
        scanf("%*c");
        return -1;
    }
    scanf("%*[^\n]");
    scanf("%*c");
    return sel - 1;
}

Playlist *unionPlaylists(Playlist *p1, Playlist *p2)
{
    Playlist *result = createEmptyPlaylist();
    if (!result)
        return NULL;
    Song *a = p1->head, *b = p2->head;
    while (a && b)
    {
        if (a->id < b->id)
        {
            addSong(result, a->id, a->title, a->artist);
            a = a->next;
        }
        else if (a->id > b->id)
        {
            addSong(result, b->id, b->title, b->artist);
            b = b->next;
        }
        else
        {
            addSong(result, a->id, a->title, a->artist);
            a = a->next;
            b = b->next;
        }
    }
    while (a)
    {
        addSong(result, a->id, a->title, a->artist);
        a = a->next;
    }
    while (b)
    {
        addSong(result, b->id, b->title, b->artist);
        b = b->next;
    }
    return result;
}

Playlist *intersectionPlaylists(Playlist *p1, Playlist *p2)
{
    Playlist *result = createEmptyPlaylist();
    if (!result)
        return NULL;
    Song *a = p1->head, *b = p2->head;
    while (a && b)
    {
        if (a->id < b->id)
            a = a->next;
        else if (a->id > b->id)
            b = b->next;
        else
        {
            addSong(result, a->id, a->title, a->artist);
            a = a->next;
            b = b->next;
        }
    }
    return result;
}

Playlist *differencePlaylists(Playlist *p1, Playlist *p2)
{
    Playlist *result = createEmptyPlaylist();
    if (!result)
        return NULL;
    Song *a = p1->head, *b = p2->head;
    while (a && b)
    {
        if (a->id < b->id)
        {
            addSong(result, a->id, a->title, a->artist);
            a = a->next;
        }
        else if (a->id > b->id)
        {
            b = b->next;
        }
        else
        {
            a = a->next;
            b = b->next;
        }
    }
    while (a)
    {
        addSong(result, a->id, a->title, a->artist);
        a = a->next;
    }
    return result;
}

Playlist *symDifferencePlaylists(Playlist *p1, Playlist *p2)
{
    Playlist *result = createEmptyPlaylist();
    if (!result)
        return NULL;
    Song *a = p1->head, *b = p2->head;
    while (a && b)
    {
        if (a->id < b->id)
        {
            addSong(result, a->id, a->title, a->artist);
            a = a->next;
        }
        else if (a->id > b->id)
        {
            addSong(result, b->id, b->title, b->artist);
            b = b->next;
        }
        else
        {
            a = a->next;
            b = b->next;
        }
    }
    while (a)
    {
        addSong(result, a->id, a->title, a->artist);
        a = a->next;
    }
    while (b)
    {
        addSong(result, b->id, b->title, b->artist);
        b = b->next;
    }
    return result;
}

void displayAndFree(Playlist *result)
{
    if (result->size == 0)
        printf("Result is empty.\n");
    else
        displayPlaylist(result);
    freePlaylist(result);
    free(result);
}

// --- Free History ---

void freeHistory()
{
    while (historyTop)
    {
        HistoryNode *temp = historyTop;
        historyTop = historyTop->next;
        free(temp);
    }
    historyCount = 0;
}

// --- Menu ---

void printMenu()
{
    printf("\n=========================================\n");
    printf("   Music Playlist Management System\n");
    printf("=========================================\n");
    printf("  Active: %s | Songs: %d | Repeat: %s\n",
           active->name, active->size, repeatMode ? "ON" : "OFF");
    printf("-----------------------------------------\n");
    printf("  1.  Add Song\n");
    printf("  2.  Delete Song\n");
    printf("  3.  Display Playlist (by SongID)\n");
    printf("  4.  Display Playlist (by Title)\n");
    printf("  5.  Display Playlist (by Artist)\n");
    printf("  6.  Display Playlist (by Artist, then Title)\n");
    printf("  7.  Search by SongID\n");
    printf("  8.  Search by Title\n");
    printf("  9.  Search by Artist\n");
    printf(" 10.  Play Next Song\n");
    printf(" 11.  Play Previous Song\n");
    printf(" 12.  Shuffle Playlist\n");
    printf(" 13.  Toggle Repeat Mode\n");
    printf(" 14.  Show Now Playing\n");
    printf(" 15.  Repeat/Display Playlist\n");
    printf(" 16.  Display History (Chronological)\n");
    printf(" 17.  Display History (Reverse)\n");
    printf(" 18.  Switch Active Playlist\n");
    printf(" 19.  Create New Playlist\n");
    printf(" 20.  List All Playlists\n");
    printf(" 21.  Union of Playlists\n");
    printf(" 22.  Intersection of Playlists\n");
    printf(" 23.  Difference (P1 - P2)\n");
    printf(" 24.  Symmetric Difference\n");
    printf(" 25.  Restore Sorted Order\n");
    printf("  0.  Exit\n");
    printf("-----------------------------------------\n");
    printf("Enter choice: ");
}

// --- Main ---

int main()
{
    srand((unsigned)time(NULL));

    // --- Requirement 4: Pre-load two playlists with 10 songs each ---

    Playlist *pl1 = createNamedPlaylist("Rock Classics");
    addPlaylistToSystem(pl1);
    addSong(pl1, 101, "Bohemian Rhapsody", "Queen");
    addSong(pl1, 102, "Stairway to Heaven", "Led Zeppelin");
    addSong(pl1, 103, "Hotel California", "Eagles");
    addSong(pl1, 104, "Comfortably Numb", "Pink Floyd");
    addSong(pl1, 105, "Sweet Child O Mine", "Guns N Roses");
    addSong(pl1, 106, "Back in Black", "AC/DC");
    addSong(pl1, 107, "Smells Like Teen Spirit", "Nirvana");
    addSong(pl1, 108, "Wish You Were Here", "Pink Floyd");
    addSong(pl1, 109, "Paint It Black", "The Rolling Stones");
    addSong(pl1, 110, "Thunderstruck", "AC/DC");

    Playlist *pl2 = createNamedPlaylist("Pop Hits");
    addPlaylistToSystem(pl2);
    addSong(pl2, 201, "Shape of You", "Ed Sheeran");
    addSong(pl2, 202, "Blinding Lights", "The Weeknd");
    addSong(pl2, 203, "Uptown Funk", "Bruno Mars");
    addSong(pl2, 204, "Rolling in the Deep", "Adele");
    addSong(pl2, 205, "Bad Guy", "Billie Eilish");
    addSong(pl2, 103, "Hotel California", "Eagles");
    addSong(pl2, 206, "Levitating", "Dua Lipa");
    addSong(pl2, 107, "Smells Like Teen Spirit", "Nirvana");
    addSong(pl2, 207, "Dynamite", "BTS");
    addSong(pl2, 208, "Stay", "The Kid LAROI");

    active = pl1;
    activeIndex = 0;

    printf("Pre-loaded '%s' with %d songs.\n", pl1->name, pl1->size);
    printf("Pre-loaded '%s' with %d songs.\n", pl2->name, pl2->size);

    // --- Main menu loop (no break, no switch, no flushInput) ---

    int choice = -1;
    int id;
    char title[MAX_LEN], artist[MAX_LEN];

    do
    {
        printMenu();
        if (scanf("%d", &choice) != 1)
        {
            printf("Invalid input! Please enter a number.\n");
            scanf("%*[^\n]");
            scanf("%*c");
            choice = -1;
        }
        else if (choice == 1)
        {
            printf("Enter Song ID: ");
            if (scanf("%d", &id) != 1)
            {
                printf("Invalid ID!\n");
                scanf("%*[^\n]");
                scanf("%*c");
            }
            else
            {
                scanf("%*c"); /* consume newline after ID */
                printf("Enter Title: ");
                readString(title, MAX_LEN);
                printf("Enter Artist: ");
                readString(artist, MAX_LEN);
                if (strlen(title) == 0 || strlen(artist) == 0)
                {
                    printf("Title and Artist cannot be empty!\n");
                }
                else
                {
                    int res = addSong(active, id, title, artist);
                    if (res == 1)
                        printf("Song added successfully.\n");
                    else if (res == 0)
                        printf("Song with ID %d already exists!\n", id);
                }
            }
        }
        else if (choice == 2)
        {
            printf("Enter Song ID to delete: ");
            if (scanf("%d", &id) != 1)
            {
                printf("Invalid ID!\n");
                scanf("%*[^\n]");
                scanf("%*c");
            }
            else
            {
                scanf("%*[^\n]");
                scanf("%*c");
                if (!deleteSong(active, id))
                    printf("Song with ID %d not found.\n", id);
            }
        }
        else if (choice == 3)
        {
            displayPlaylist(active);
        }
        else if (choice == 4)
        {
            displaySorted(active, 1);
        }
        else if (choice == 5)
        {
            displaySorted(active, 2);
        }
        else if (choice == 6)
        {
            displaySorted(active, 3);
        }
        else if (choice == 7)
        {
            printf("Enter Song ID: ");
            if (scanf("%d", &id) != 1)
            {
                printf("Invalid ID!\n");
                scanf("%*[^\n]");
                scanf("%*c");
            }
            else
            {
                scanf("%*[^\n]");
                scanf("%*c");
                searchByID(active, id);
            }
        }
        else if (choice == 8)
        {
            scanf("%*c"); /* consume newline */
            printf("Enter Title: ");
            readString(title, MAX_LEN);
            searchByTitle(active, title);
        }
        else if (choice == 9)
        {
            scanf("%*c"); /* consume newline */
            printf("Enter Artist: ");
            readString(artist, MAX_LEN);
            searchByArtist(active, artist);
        }
        else if (choice == 10)
        {
            playNext(active);
        }
        else if (choice == 11)
        {
            playPrev(active);
        }
        else if (choice == 12)
        {
            shufflePlaylist(active);
        }
        else if (choice == 13)
        {
            repeatMode = !repeatMode;
            printf("Repeat mode: %s\n", repeatMode ? "ON" : "OFF");
        }
        else if (choice == 14)
        {
            showNowPlaying(active);
        }
        else if (choice == 15)
        {
            repeatDisplay(active);
        }
        else if (choice == 16)
        {
            displayHistoryChrono();
        }
        else if (choice == 17)
        {
            displayHistoryReverse();
        }
        else if (choice == 18)
        {
            int sel = selectPlaylist("Select playlist to switch to: ");
            if (sel >= 0)
            {
                activeIndex = sel;
                active = playlists[activeIndex];
                printf("Switched to '%s'.\n", active->name);
            }
        }
        else if (choice == 19)
        {
            if (playlistCount >= MAX_PLAYLISTS)
            {
                printf("Maximum number of playlists reached!\n");
            }
            else
            {
                scanf("%*c"); /* consume newline */
                printf("Enter name for new playlist: ");
                char pname[MAX_LEN];
                readString(pname, MAX_LEN);
                if (strlen(pname) == 0)
                {
                    printf("Playlist name cannot be empty!\n");
                }
                else
                {
                    Playlist *np = createNamedPlaylist(pname);
                    if (np)
                    {
                        addPlaylistToSystem(np);
                        printf("Playlist '%s' created (Playlist #%d).\n", pname, playlistCount);
                    }
                }
            }
        }
        else if (choice == 20)
        {
            listAllPlaylists();
        }
        else if (choice == 21)
        {
            printf("--- Union of Two Playlists ---\n");
            int s1 = selectPlaylist("Select first playlist: ");
            if (s1 >= 0)
            {
                int s2 = selectPlaylist("Select second playlist: ");
                if (s2 >= 0)
                {
                    Playlist *r = unionPlaylists(playlists[s1], playlists[s2]);
                    printf("\n--- Union of '%s' and '%s' ---\n", playlists[s1]->name, playlists[s2]->name);
                    displayAndFree(r);
                }
            }
        }
        else if (choice == 22)
        {
            printf("--- Intersection of Two Playlists ---\n");
            int s1 = selectPlaylist("Select first playlist: ");
            if (s1 >= 0)
            {
                int s2 = selectPlaylist("Select second playlist: ");
                if (s2 >= 0)
                {
                    Playlist *r = intersectionPlaylists(playlists[s1], playlists[s2]);
                    printf("\n--- Intersection of '%s' and '%s' ---\n", playlists[s1]->name, playlists[s2]->name);
                    displayAndFree(r);
                }
            }
        }
        else if (choice == 23)
        {
            printf("--- Difference (P1 - P2) ---\n");
            int s1 = selectPlaylist("Select P1: ");
            if (s1 >= 0)
            {
                int s2 = selectPlaylist("Select P2: ");
                if (s2 >= 0)
                {
                    Playlist *r = differencePlaylists(playlists[s1], playlists[s2]);
                    printf("\n--- Difference '%s' - '%s' ---\n", playlists[s1]->name, playlists[s2]->name);
                    displayAndFree(r);
                }
            }
        }
        else if (choice == 24)
        {
            printf("--- Symmetric Difference ---\n");
            int s1 = selectPlaylist("Select first playlist: ");
            if (s1 >= 0)
            {
                int s2 = selectPlaylist("Select second playlist: ");
                if (s2 >= 0)
                {
                    Playlist *r = symDifferencePlaylists(playlists[s1], playlists[s2]);
                    printf("\n--- Symmetric Difference of '%s' and '%s' ---\n", playlists[s1]->name, playlists[s2]->name);
                    displayAndFree(r);
                }
            }
        }
        else if (choice == 25)
        {
            restoreSortedOrder(active);
        }
        else if (choice == 0)
        {
            printf("Goodbye!\n");
        }
        else
        {
            printf("Invalid choice! Enter 0-25.\n");
        }
    } while (choice != 0);

    // Cleanup all playlists
    int i;
    for (i = 0; i < playlistCount; i++)
    {
        freePlaylist(playlists[i]);
        free(playlists[i]);
    }
    freeHistory();
    return 0;
}
