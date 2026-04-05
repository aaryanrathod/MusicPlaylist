#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LEN 100

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
Playlist pl1 = {NULL, NULL, NULL, 0};
Playlist pl2 = {NULL, NULL, NULL, 0};
Playlist *active;
HistoryNode *historyTop = NULL;
int historyCount = 0;
int repeatMode = 0;

// --- Utility ---

void flushInput()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

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
    if (sublen == 0)
        return 1;
    for (i = 0; i <= slen - sublen; i++)
    {
        int match = 1;
        for (j = 0; j < sublen; j++)
        {
            if (toLowerChar(str[i + j]) != toLowerChar(sub[j]))
            {
                match = 0;
                break;
            }
        }
        if (match)
            return 1;
    }
    return 0;
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
    return pl;
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
           (active == &pl1) ? "Playlist 1" : "Playlist 2",
           active->size, repeatMode ? "ON" : "OFF");
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
    printf(" 19.  Union of Playlists\n");
    printf(" 20.  Intersection of Playlists\n");
    printf(" 21.  Difference (P1 - P2)\n");
    printf(" 22.  Symmetric Difference\n");
    printf("  0.  Exit\n");
    printf("-----------------------------------------\n");
    printf("Enter choice: ");
}

// --- Main ---

int main()
{
    srand((unsigned)time(NULL));
    active = &pl1;

    int choice, id;
    char title[MAX_LEN], artist[MAX_LEN];

    do
    {
        printMenu();
        if (scanf("%d", &choice) != 1)
        {
            printf("Invalid input! Please enter a number.\n");
            flushInput();
            continue;
        }
        flushInput();

        switch (choice)
        {
        case 1:
            printf("Enter Song ID: ");
            if (scanf("%d", &id) != 1)
            {
                printf("Invalid ID!\n");
                flushInput();
                break;
            }
            flushInput();
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
            break;

        case 2:
            printf("Enter Song ID to delete: ");
            if (scanf("%d", &id) != 1)
            {
                printf("Invalid ID!\n");
                flushInput();
                break;
            }
            flushInput();
            if (!deleteSong(active, id))
                printf("Song with ID %d not found.\n", id);
            break;

        case 3:
            displayPlaylist(active);
            break;
        case 4:
            displaySorted(active, 1);
            break;
        case 5:
            displaySorted(active, 2);
            break;
        case 6:
            displaySorted(active, 3);
            break;

        case 7:
            printf("Enter Song ID: ");
            if (scanf("%d", &id) != 1)
            {
                printf("Invalid ID!\n");
                flushInput();
                break;
            }
            flushInput();
            searchByID(active, id);
            break;

        case 8:
            printf("Enter Title: ");
            readString(title, MAX_LEN);
            searchByTitle(active, title);
            break;

        case 9:
            printf("Enter Artist: ");
            readString(artist, MAX_LEN);
            searchByArtist(active, artist);
            break;

        case 10:
            playNext(active);
            break;
        case 11:
            playPrev(active);
            break;
        case 12:
            shufflePlaylist(active);
            break;

        case 13:
            repeatMode = !repeatMode;
            printf("Repeat mode: %s\n", repeatMode ? "ON" : "OFF");
            break;

        case 14:
            showNowPlaying(active);
            break;
        case 15:
            repeatDisplay(active);
            break;
        case 16:
            displayHistoryChrono();
            break;
        case 17:
            displayHistoryReverse();
            break;

        case 18:
            active = (active == &pl1) ? &pl2 : &pl1;
            printf("Switched to %s.\n", (active == &pl1) ? "Playlist 1" : "Playlist 2");
            break;

        case 19:
        {
            Playlist *r = unionPlaylists(&pl1, &pl2);
            printf("\n--- Union of Playlist 1 and Playlist 2 ---\n");
            displayAndFree(r);
            break;
        }
        case 20:
        {
            Playlist *r = intersectionPlaylists(&pl1, &pl2);
            printf("\n--- Intersection ---\n");
            displayAndFree(r);
            break;
        }
        case 21:
        {
            Playlist *r = differencePlaylists(&pl1, &pl2);
            printf("\n--- Difference (P1 - P2) ---\n");
            displayAndFree(r);
            break;
        }
        case 22:
        {
            Playlist *r = symDifferencePlaylists(&pl1, &pl2);
            printf("\n--- Symmetric Difference ---\n");
            displayAndFree(r);
            break;
        }

        case 0:
            printf("Goodbye!\n");
            break;
        default:
            printf("Invalid choice! Enter 0-22.\n");
            break;
        }
    } while (choice != 0);

    // Cleanup
    freePlaylist(&pl1);
    freePlaylist(&pl2);
    freeHistory();
    return 0;
}
