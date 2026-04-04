# Music Playlist Management System

A simple, single-file menu-driven Music Playlist Management System written in pure C. This project simulates a music player using basic Data Structures (Doubly Linked List for playlists and Stack via Singly Linked List for play history).

## Features

1. **Playlist Operations (Doubly Linked List)**:
   - Add a song (kept sorted by SongID).
   - Delete a song by its SongID.
   - Display the playlist sorted by: SongID, Title, Artist, or Artist then Title.
   - Search for a song by SongID, Title or Artist.

2. **Player Controls**:
   - Play the next song or previous song.
   - Toggle shuffle mode (randomizes the active playlist).
   - Toggle repeat mode (loops the playlist).
   - Show currently playing song.

3. **Play History (Stack via Singly Linked List)**:
   - Records every played song.
   - Display history in chronological order (oldest first).
   - Display history in reverse chronological order (most recent first).

4. **Multi-Playlist Set Operations**:
   - Maintain and switch between two independent playlists.
   - Compute Union of playlists (all unique songs).
   - Compute Intersection of playlists (common songs).
   - Compute Difference (songs in Playlist 1 but not Playlist 2).
   - Compute Symmetric Difference (songs in exactly one playlist).

## Compilation and Execution

There are no external dependencies other than standard C libraries (`stdio.h`, `stdlib.h`, `string.h`, `time.h`).

Compile the code with GCC or any standard C compiler:

```bash
gcc -o music_player main.c -Wall -Wextra
```

Run the program:

```bash
./music_player
```
