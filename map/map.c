/*
* Map.c is a file which contains all the information about map
* Izzy / Destin - 2024
*/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "../libcs50/file.h"
#include "../libcs50/hashtable.h"
#include "../libcs50/set.h"
#include "spot.h"
#include "map.h"
#include "person.h"
#include <unistd.h> 
#include <arpa/inet.h>

#define GoldTotal 250      // amount of gold in the game
#define GoldMinNumPiles 10 // minimum number of gold piles
#define GoldMaxNumPiles 30 // maximum number of gold piles

typedef struct map{ //Contains the rows and columns of the grid, and a grid of objects and a grid of players
    int rows;
    int columns;
    spot_t** grid;
    person_t** players; 
} map_t;


map_t* map_new(char* path){
    map_t* map = calloc(1, sizeof(map_t)); //Allocates memory for map
    if(map == NULL){
        fprintf(stderr, "Memory allocation failure");
        exit(1);
    }
    FILE* in = NULL;  
    in = fopen(path, "r"); //Opens file
    if (in == NULL) {
        fprintf(stderr, "Error opening file %s\n", path);
        exit(1);
    }
    fprintf(stderr, "Found file %s\n", path);
    map_validate(map, in); //Find rows and columns of file
    fclose(in); //Close in
    in = fopen(path, "r"); //Reopen 'in'
    map_load(map, in); //Load 'in' map
    fclose(in); //Close 'in'
    return map;
}

void map_validate(map_t* map, FILE* in){
    char* line = NULL;
    int numRows = 0; 
    int numColumns = 0;
    while((line = file_readLine(in)) != NULL){ //Read through each line of file
        int line_size = (int)strlen(line); //Find size of line
        if(line_size > numColumns){ //Determining maximum line size (equals number of columns)
            numColumns = line_size;
        }
        numRows++; 
        free(line);
    }
    map->grid = calloc((numColumns*numRows), sizeof(spot_t*)); //Create memory for grid
    map->players = calloc((numColumns*numRows), sizeof(person_t*)); //Create memory for players
    for (int i = 0; i < (map->rows*map->columns); i++) { //Going through array and setting value to NULL
        map->players[i] = NULL; 
        map->grid[i] = NULL;
    }
    if(map->players == NULL || map->grid == NULL){
        fprintf(stderr, "Memory allocation failed");
        exit(1);
    }
    map->columns = numColumns;
    map->rows = numRows;
    free(line);
}

void map_load(map_t* map, FILE* in){ //Load in map data
    char c;
    char* line = NULL;
    int current_row = 0; 
    while((line = file_readLine(in)) != NULL){  //Go through entire file
        int i = 0;
        while ((c = line[i]) != '\0') { //Parse character out of line
            spot_t* spot = spot_new(); //Create new spot
            c = line[i]; //Current character in line
            if(!spot_insert(spot, c)){ //Test spot insert
                fprintf(stderr, "Error inserting item %c", c);
                exit(1);
            }
            map->grid[i+(current_row*map->columns)] = spot; //Setting current position on grid to spot
            i++;
        }
        current_row++;
        free(line);
    }
    free(line);
}

void map_print(map_t* map, FILE* out){ 
    for(int row = 0; row < map->rows; row++){ //Go through rows
        for(int column = 0; column < map->columns; column++){ //Go through columns
            if(map->grid[column + (row*map->columns)] == NULL){  //If current position is null do new line
                fprintf(out, "%c", '\n');
                break;
            }
            else if(map->players[column + (row*map->columns)] != NULL){ //Found a player
                person_t* person = map->players[column + (row*map->columns)]; //Get player there
                fprintf(out, "%c", person_getLetter(person)); //Print player (all players are visible but could be changed)
            }else{
                spot_t* spot = map->grid[column + (row*map->columns)]; //Get the spot
                if(!get_visibility(spot)){ //Check spot visibility
                    // modify this to account for visible gold.
                    fprintf(out, "%c", spot_item(spot));
                }
                else{ //Print space
                    fprintf(out, "%c", ' ');
                }
            }
        }
        fprintf(out, "%c", '\n');
    }
}

void map_delete(map_t* map){
    for(int i = 0; i < (map->columns*map->rows); i++){ //Go through entire grid
        if(map->grid[i] != NULL){
            spot_delete(map->grid[i]); //Delete spot
        }
        if(map->players[i] != NULL){
            person_delete(map->players[i]); //Delete player
        }
    }
    free(map->grid);
    free(map->players);
    free(map);
}

bool move_person(map_t* map, person_t* person, char direction){
    int current_pos = person_getPos(person); //Current position
    int row = (int) (current_pos / map->columns); //Find current row
    int column = current_pos - (row*map->columns); //Find current column
    int new_pos = 0;
    if(direction == 'k'){ //Up
        if(row == 0){
            return false;
        }
        new_pos = current_pos - map->columns; 
    }
    else if(direction == 'h'){ //Left
        if(column == 0){
            return false;
        }
        new_pos = current_pos - 1;
    }
    else if(direction == 'j'){ //Down
        if(row == map->rows){ 
            return false;
        }
        new_pos = current_pos + map->columns;
    }
    else if(direction == 'l'){ //Right
        if(column == map->columns){ 
            return false;
        }
        new_pos = current_pos + 1;
    }
    else if(direction == 'y'){ //Up left
        if(column == 0 || row == 0){
            return false;
        }
        new_pos = current_pos - map->columns - 1;
    }
    else if(direction == 'u'){ //Up right
        if(column == map->columns || row == 0){ 
            return false;
        }
        new_pos = current_pos - map->columns + 1;
    }
    else if(direction == 'b'){ //Down left
        if(column == 0 || row == map->rows){ 
            return false;
        }
        new_pos = current_pos + map->columns - 1;
    }
    else if(direction == 'n'){ //Down right
        if(column == map->columns || row == map->rows){ 
            return false;
        }
        new_pos = current_pos + map->columns + 1;
    }
    
    if(map->players[new_pos] != NULL){ //If there is a player in the way
        person_setPos(map->players[new_pos], current_pos); //Switching person 
        person_setPos(person, new_pos); //Going to new position
        person_t *temp = map->players[new_pos]; //Creating temporary person
        map->players[new_pos] = person; 
        map->players[current_pos] = temp;

    }
    else if(spot_item(map->grid[new_pos]) == '.' || spot_item(map->grid[new_pos]) == '#' || spot_item(map->grid[new_pos]) == '*'){ //Valid movment square
        person_setPos(person, new_pos);  //Move person
        map->players[current_pos] = NULL; 
        map->players[new_pos] = person;
        if(spot_item(map->grid[new_pos]) == '*'){ //Found gold
            int gold = spot_get_gold(map->grid[new_pos]);
            fprintf(stdout, "Gold on this spot was: %d\n", gold);
            person_addGold(person, gold); //Add gold to player count
            spot_insert(map->grid[new_pos], '.'); //Inserts '.' in its place 
        }
    }
    else{
        return false;
    }
    return true;
}

set_t* get_freeSpace(map_t * map, int* num_spaces){
    set_t* toReturn = set_new(); //Define the set of free spaces
    for(int i = 0; i < (map->columns*map->rows); i++){ //Go through all grid spots
        if((spot_item(map->grid[i]) == '.') && map->players[i] == NULL && (spot_get_gold(map->grid[i])== 0)){ //No players here and '.' present and no gold
            char key[20];
            sprintf(key, "%d", *num_spaces);  
            int* value = malloc(sizeof(int)); 
            if (value != NULL) { 
                *value = i; //Going through each key in map
                set_insert(toReturn, key, value); //Adds possible key to the set with current index
            } else {
                fprintf(stderr, "Memory failure");
            }
            (*num_spaces)++;
        }
    }
    return toReturn; //Returning set of blank spaces
}

person_t* insert_person(map_t* map, char c, char* name, addr_t address){ //The idea is to insert all possible position into a set and extract one randomly
    set_t* indexes;
    int num_spaces = 0;
    int final_index = 0;
    indexes = get_freeSpace(map, &num_spaces);
    if(num_spaces == 0){
        return NULL;
    }
    int random_number = rand() % (num_spaces); //Generate random number from 0 to num_spaces (possible values in set)
    char random_string[20];
    sprintf(random_string, "%d", random_number); //Turn random number to string
    int* temp = set_find(indexes, random_string); //Finds that string in the set
    final_index = *temp; //Sets the index associated with that value
    person_t* person = person_new(c, name, address, get_columns(map) *get_rows(map)); //Create a new person with name 'c' and name 'name'
    person_setPos(person, final_index); //Set position in person struct
    map->players[final_index] = person; //Set position in map
    set_delete(indexes, namedelete); //Deletes set 
    return person;
}

void namedelete(void* item) //Deletes a name as helper function for hashtable
{
  if (item != NULL) {
    free(item);   
  }
}

void gold_initialize(map_t* map)
{
    int compare_gold = 0;
    int random_piles = rand() % (GoldMaxNumPiles + 1 - GoldMinNumPiles) + GoldMinNumPiles;  //Defining the number of random piles
    int gold_remaining = GoldTotal - random_piles; // each pile must have at least one gold
    for(int pile = 0; pile<random_piles; pile++){ //Go through each pile
        
        int random_gold = rand() % gold_remaining; 
        if(pile == random_piles - 1){ //Last pile add remaining gold
            random_gold = gold_remaining;
        }
        int space_count = 0;
        set_t* indices = get_freeSpace(map, &space_count);
        int random_index = rand() % space_count; //Define a random index in the random piles
        gold_remaining -= random_gold; //Subtract the random gold defined from total remaining
        char random_number_string[20];
        sprintf(random_number_string, "%d", random_index); //Turn into a string
        int* position = set_find(indices, random_number_string); //Find that space given random number
        spot_t * spot = map->grid[*position];
        spot_add_gold(spot, random_gold + 1); //Adds gold and ensure that tehre is at eleast one
        compare_gold += random_gold + 1;
        spot_set_item(spot, '*'); //Sets spot to gold character
        set_delete(indices, namedelete);
    }
}



// clone the current map. To be called whenever inserting a new player
map_t* clone_map(map_t* current_map) {
    if (current_map == NULL) {
        return NULL;
    }
    
    map_t* new_map = malloc(sizeof(map_t));
    if (new_map == NULL) {
        fprintf(stderr, "Memory allocation failure");
        exit(1);
    }

    new_map->rows = current_map->rows; //Copying over rows 
    new_map->columns = current_map->columns; //Copying over columns

    new_map->grid = malloc(current_map->rows * current_map->columns * sizeof(spot_t*)); //Allocating memory for grid
    new_map->players = malloc(current_map->rows * current_map->columns * sizeof(person_t*)); //Allocating memory for players

    if (new_map->grid == NULL || new_map->players == NULL) {
        fprintf(stderr, "Memory allocation failure");
        exit(1);
    }

    // Clone grid
    for (int i = 0; i < current_map->rows * current_map->columns; i++) {
        if (current_map->grid[i] != NULL) {
            new_map->grid[i] = spot_clone(current_map->grid[i], i, 0); // Assuming spot_clone is a function that correctly clones a spot
        } else {
            exit(1);
        }
    }
    // Clone players
    for (int i = 0; i < current_map->rows * current_map->columns; i++) {
        if (current_map->players[i] != NULL) {
            new_map->players[i] = person_clone(current_map->players[i]); // Assuming person_clone is a function that correctly clones a person
        } else {
            new_map->players[i] = NULL;
        }
    }
    return new_map;
}

person_t** get_players(map_t* map){
    return map->players;
}

int get_rows(map_t * map){
    return map->rows;
}

int get_columns(map_t* map){
    return map->columns;
}

char* grid_to_string_spectator(map_t* map){
    int total_size = (map->rows * (map->columns + 1)) + 1; // Including newlines and null terminator
    char *to_return = (char *)malloc(total_size * sizeof(char));
    if (!to_return) {
        return NULL;
    }
    to_return[0] = '\0'; // Initialize to empty string

    for(int index = 0; index < map->rows * map->columns; index++){
        char to_append;
        if(map->players[index] != NULL){
            to_append = person_getLetter(map->players[index]);
        }else{
            to_append = spot_item(map->grid[index]);
        }
        if(index % map->columns == 0){
            strncat(to_return, "\n", 2);
        }
        strncat(to_return, &to_append, 1);
    }
    return to_return;
}

char* grid_to_string_player(map_t* map, char letter) {
    int total_size = (map->rows * (map->columns + 1)) + 1; // Including newlines and null terminator
    char *to_return = (char *)malloc(total_size * sizeof(char)); //Allocating memory for map string
    if (!to_return) { //Validating not NULL
        return NULL;
    }
    to_return[0] = '\0'; // Initialize to empty string
    for(int index = 0; index < map->rows * map->columns; index++){ //Run through map
        char to_append;
        if(map->players[index] != NULL){
            to_append = (letter == person_getLetter(map->players[index])) ? '@' : person_getLetter(map->players[index]); //Replace the person with @
        }else{
            to_append = spot_item(map->grid[index]); //Appending spot
            if(spot_invisible_gold(map->grid[index])){ //Ensuring invisible gold remains invisible
                to_append = '.';
            }
        }
        if(index % map->columns == 0){
            strncat(to_return, "\n", 2);
        }
        if(!get_visibility(map->grid[index])){ //Getting visibility and applying it to map
            to_append = ' ';
        }
        strncat(to_return, &to_append, 1);
    }
    return to_return;
}

void set_person(map_t* map, person_t* person){
    if(person == NULL){
        fprintf(stderr, "The person us null\n");
    }
    map->players[person_getPos(person)] = person_clone(person); //Cloning the person
}

spot_t** get_grid(map_t* map){
    return map->grid;
}