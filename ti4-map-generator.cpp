#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <time.h>
#include <exception>
#include <cmath>
#include <iterator>
#include <sstream>

#include "json.hpp"
#include "cxxopts.hpp"

#define BACKWARD_HAS_BFD 1
#include "backward-cpp/backward.hpp"

using json = nlohmann::json;

using namespace std;

enum PlanetTrait
{
    NO_TRAIT,
    CULTURAL,
    INDUSTRIAL,
    HAZARDOUS
};

static map<string, PlanetTrait> trait_key = {
    {"NO_TRAIT", NO_TRAIT},
    {"CULTURAL", CULTURAL},
    {"HAZARDOUS", HAZARDOUS},
    {"INDUSTRIAL", INDUSTRIAL}
};

enum TechColor
{
    NO_TECH,
    BLUE,
    RED,
    GREEN,
    YELLOW
};

static map<string, TechColor> tech_key = {
    {"NO_TECH", NO_TECH},
    {"BLUE", BLUE},
    {"RED", RED},
    {"GREEN", GREEN},
    {"YELLOW", YELLOW}
};

enum Wormhole
{
    NO_WORMHOLE,
    ALPHA,
    BETA,
    DELTA
};

static map<string, Wormhole> wormhole_key = {
    {"NO_WORMHOLE", NO_WORMHOLE},
    {"ALPHA", ALPHA},
    {"BETA", BETA},
    {"DELTA", DELTA}
};

enum Anomaly
{
    NO_ANOMALY,
    EMPTY,
    GRAVITY_RIFT,
    NEBULA,
    SUPERNOVA,
    ASTEROID_FIELD
};

static map<string, Anomaly> anomaly_key = {
    {"NO_ANOMALY", NO_ANOMALY},
    {"EMPTY", EMPTY},
    {"GRAVITY_RIFT", GRAVITY_RIFT},
    {"NEBULA", NEBULA},
    {"SUPERNOVA", SUPERNOVA},
    {"ASTEROID_FIELD", ASTEROID_FIELD},
};

typedef struct Planet
{
    string name;
    int resources;
    int influence;
    PlanetTrait trait;
    TechColor tech;
} Planet;

typedef struct Location
{
    int i;
    int j;

    Location operator+(Location a) {
        struct Location new_location;
        new_location.i = this->i + a.i;
        new_location.j = this->j + a.j;
        return new_location;
    }
} Location;



class Tile
{
    int number;
    Wormhole wormhole;
    list<Planet> planets;
    Anomaly anomaly;
    Location location;
    string race;

    int resources;
    int influence;

    public:
    Tile(int);
    Tile(int, string);
    Tile(int, list<Planet>, Wormhole, Anomaly);
    Tile(int, list<Planet>, string race);
    string get_description_string() const;
    int get_number();
    Wormhole get_wormhole();
    Anomaly get_anomaly();
    TechColor get_techcolor();
    void set_location(Location);
    Location get_location();
    int get_resource_value();
    int get_influence_value();
    bool is_home_system();
};

int Tile::get_resource_value()
{
    return resources;
}

int Tile::get_influence_value()
{
    return influence;
}

int Tile::get_number() {
    return number;
}

Anomaly Tile::get_anomaly() {
    return anomaly;
}

TechColor Tile::get_techcolor() {
   for (auto p : planets) {
       if (p.tech) {
           return p.tech;
       }
   }
   return NO_TECH;
}

Wormhole Tile::get_wormhole() {
    return wormhole;
}

void Tile::set_location(Location l) {
    location = l;
}

Location Tile::get_location() {
    return location;
}

string Tile::get_description_string() const {
    ostringstream desc;
    desc << "Tile " << number << " - ";
    for (auto it = planets.begin(); it != planets.end(); it++) {
        if (it != planets.begin()) {
            desc << ", ";
        }
        desc << it->name;
    }

    return desc.str();
}

bool Tile::is_home_system()
{
    return race.length();
}

std::ostream& operator<< (std::ostream &out, Tile const& tile) {
    out << tile.get_description_string();
    return out;
}

Tile::Tile(int n) : number(n), wormhole(NO_WORMHOLE), anomaly(NO_ANOMALY), race("") {};

Tile::Tile(int n, string race) 
    : number(n), wormhole(NO_WORMHOLE), anomaly(NO_ANOMALY), race(race) {};

Tile::Tile(int n, list<Planet> p, Wormhole w, Anomaly a)
{
    number = n;
    planets = p;
    wormhole = w;
    anomaly = a;
    race = "";

    resources = 0;
    influence = 0;
    for (auto planet : planets) {
        resources += planet.resources;
        influence += planet.influence;
    }
}

Tile::Tile(int n, list<Planet> p, string race1)
{
    number = n;
    planets = p;
    wormhole = NO_WORMHOLE;
    anomaly = NO_ANOMALY;
    race = race1;

    resources = 0;
    influence = 0;
    for (auto planet : planets) {
        resources += planet.resources;
        influence += planet.influence;
    }
}

typedef map<Tile*,map<Tile*, float>> double_tile_map;

enum HomeSystemSetups {DUMMY, RANDOM_RACES, CHOSEN_RACES};

class Galaxy
{
    list<Tile> tiles;
    Tile *grid[7][7]; // Locations of tiles
    Tile *mecatol;
    list<Tile*> home_systems;
    list<Tile*> movable_systems;
    list<Tile*> red_tiles;
    list<Tile*> blue_tiles;
    map<Wormhole, list<Tile*>> wormhole_systems;
    Tile boundary_tile; // used for inaccesable locations in the grid
    map<string, float> evaluate_options;

    void import_tiles(string tile_filename);
    void random_home_tiles(int n);
    void dummy_home_tiles(int n);
    void chosen_home_tiles(int n, string chosen);
    void initialize_grid(int n, string mandatory_tile_numbers);
    void place_tile(Location location, Tile*);
    void swap_tiles(Tile *, Tile *);
    int count_adjacent_anomalies();
    int count_adjacent_wormholes();
    Tile* get_tile_at(Location location);
    list<Tile*> get_adjacent(Tile* t1);
    map<Tile*, float> distance_to_other_tiles(Tile* t1);
    double_tile_map calculate_stakes(double_tile_map distances);
    vector<pair<Tile*, Tile*>> make_swap_list();
    bool is_wormhole_near_creuss(int near_dist, double_tile_map distances);
    bool is_supernova_near_muaat(int near_dist, double_tile_map distances);
    bool winnu_have_clear_path_to_mecatol(double_tile_map distances);


    public:
    Galaxy(string tile_filename, int n_players, HomeSystemSetups, 
            string home_tile_ids, string mandatory_tile_numbers);
    void print_grid();
    void print_distances_from(int);
    void set_evaluate_option(string name, float val);
    float evaluate_grid();
    void optimize_grid();
    void write_json(string filename);
};

Galaxy::Galaxy(string tile_filename, int n_players, HomeSystemSetups hss, 
        string home_tile_numbers, string mandatory_tile_numbers)
    : boundary_tile(0)
{
    import_tiles(tile_filename);
    switch (hss) {
        case DUMMY: 
            dummy_home_tiles(n_players);
            break;
        case RANDOM_RACES:
            random_home_tiles(n_players);
            break;
        case CHOSEN_RACES:
            chosen_home_tiles(n_players, home_tile_numbers);
    }
    initialize_grid(n_players, mandatory_tile_numbers);

    //for (auto i : tiles) {
    //    cout << i << endl;
    //}
}

void Galaxy::place_tile(Location l, Tile* tile) 
{
    if (tile) {
        tile->set_location(l);
    }
    grid[l.i][l.j] = tile;
}

Planet create_planet_from_json(json j)
{
    Planet new_planet;

    new_planet.name = j["name"];
    new_planet.resources = j["resources"];
    new_planet.influence = j["influence"];

    new_planet.trait = trait_key.at(j["trait"]);
    new_planet.tech = tech_key.at(j["tech"]);

    return new_planet;
}

Tile create_tile_from_json(json j)
{
    int number = j["number"];
    Wormhole wormhole = wormhole_key.at(j["wormhole"]);
    Anomaly anomaly = anomaly_key.at(j["anomaly"]);

    // Create planet list
    json planet_list = j["planets"];
    list<Planet> planets;
    for (json::iterator it = planet_list.begin(); it != planet_list.end(); it++) {
        planets.push_back(create_planet_from_json(it.value()));
    }

    Tile new_tile = Tile(number, planets, wormhole, anomaly);
    return new_tile;
}

void Galaxy::import_tiles(string tile_filename)
{
    // Parse tile json
    // TODO error handle parse errors file not exist etc
    json tile_json;
    ifstream json_file;
    cerr << "Importing tiles from " << tile_filename << endl;
    json_file.open(tile_filename);
    try {json_file >> tile_json;} 
    catch (...) {
        cerr << "Error loading/parsing " << tile_filename << endl;
        exit(-1);
    }
    json_file.close();

    // Create list of tiles
    json tile_list = tile_json["red_tiles"];
    for (json::iterator it = tile_list.begin(); it != tile_list.end(); it++) {
        tiles.push_back(create_tile_from_json(it.value()));
        Tile *added_tile = &tiles.back();
        red_tiles.push_back(added_tile);
        if (added_tile->get_wormhole()) {
            wormhole_systems[added_tile->get_wormhole()].push_back(added_tile);
        }
    }
    tile_list = tile_json["blue_tiles"];
    for (json::iterator it = tile_list.begin(); it != tile_list.end(); it++) {
        tiles.push_back(create_tile_from_json(it.value()));
        Tile *added_tile = &tiles.back();
        blue_tiles.push_back(added_tile);
        if (added_tile->get_wormhole()) {
            wormhole_systems[added_tile->get_wormhole()].push_back(added_tile);
        }
    }
    // Also home tiles
    tile_list = tile_json["home_tiles"];
    for (json::iterator it = tile_list.begin(); it != tile_list.end(); it++) {
        tiles.push_back(create_tile_from_json(it.value()));
        Tile *added_tile = &tiles.back();
        home_systems.push_back(added_tile);
    }
    
    // Save a pointer to mecatol rex
    tiles.push_back(create_tile_from_json(tile_json["mecatol"]));
    mecatol = &tiles.back();

    cerr << "Loaded " << tiles.size() << " tiles" << endl;
}

list<Tile*> get_shuffled_list(list<Tile*> l)
{
    vector<Tile *> to_shuffle;
    list<Tile *> ret;
    for (auto it : l) {
        to_shuffle.push_back(it);
    }
    random_shuffle(to_shuffle.begin(), to_shuffle.end());
    for (auto it : to_shuffle) {
        ret.push_back(it);
    }
    return ret;
}

void Galaxy::random_home_tiles(int n) {
    auto shuffled = get_shuffled_list(home_systems);
    auto it = shuffled.begin();
    home_systems.clear();
    list<Tile*> new_home_tiles;
    for (int i = 0; i < n; i++) {
        home_systems.push_back(*it);
        it++;
    }
}

list<Tile*> get_tile_pointers(list<Tile*> tiles, string numbers)
{
    list<int> n_list;
    stringstream chosen_ss(numbers);
    int n;
    while (chosen_ss >> n) {
        cout << "using number " << n << endl;
        n_list.push_back(n);
    }
    
    list<Tile*> matching_tiles;
    for (auto t : tiles) {
        if (find(n_list.begin(), n_list.end(), t->get_number()) != n_list.end()) {
            matching_tiles.push_back(t);
        }
    }

    return matching_tiles;
}

void Galaxy::chosen_home_tiles(int n, string chosen) {

    list<Tile*> available_home_systems;
    available_home_systems = get_tile_pointers(home_systems, chosen);
    home_systems = available_home_systems;
}

void Galaxy::dummy_home_tiles(int n) {
    home_systems.clear();
    for (int i = 0; i < n; i++) {
        Tile new_tile = Tile(-i - 1, "Home System");
        tiles.push_back(new_tile);
        home_systems.push_back(&tiles.back());
    }
}

template <class t>
list<t> get_shuffled_list(list<t> l) {
    list<t> new_list;
    new_list = l;
    random_shuffle(l.begin(), l.end());
    return l;
}

// TODO this funtion is way too long and filled with hardcoded bs
void Galaxy::initialize_grid(int n_players, string mandatory_tile_numbers) {
    if (n_players < 3 or n_players > 6) {
        throw invalid_argument("Must have between 3 and 6 players");
    }

    for (int i = 0; i < 7;i++) {
        for (int j = 0; j < 7; j++) {
            place_tile({i, j}, NULL);
        }
    }
    for (int i = 0; i < 3;i++) {
        for (int j = 4 + i; j < 7; j++) {
            place_tile({i, j}, &boundary_tile);
        }
    }
    for (int j = 0; j < 3;j++) {
        for (int i = 4 + j; i < 7; i++) {
            place_tile({i, j}, &boundary_tile);
        }
    }
    if (n_players == 3) {
        list<Location> extra_removed = {{0,2},{0,3},{1,4},{2,0},{3,0},{4,1},{5,6},{6,6},{6,5}};
        for (auto l : extra_removed) {
            place_tile(l, &boundary_tile);
        }
    }

    // Place Mecatol at centre of galaxy
    place_tile({3, 3}, mecatol);

    // Place home systems // TODO for custom shapes
    vector<Location> start_positions;
    switch (n_players) {
        case 3: {vector<Location> tmp = {{0,0},{3,6},{6,3}}; start_positions = tmp; break;}
        case 4: {vector<Location> tmp = {{0,2},{4,6},{6,4},{2,0}}; start_positions = tmp; break;}
        case 5: {vector<Location> tmp = {{0,2},{6,3},{3,6},{6,6},{2,0}}; start_positions = tmp; break;}
        case 6: {vector<Location> tmp = {{0,0},{0,3},{3,6},{6,6},{6,3},{3,0}}; start_positions = tmp; break;}
    }
    int i = 0;
    for (auto it : get_shuffled_list(home_systems)) {
        place_tile(start_positions[i], it);
        i++;
    }

    // Shuffle tiles always including mandatory tiles first
    list<Tile*> random_tiles;
    int n_red, n_blue; // number of each colored tiles to include
    switch (n_players) {
        case 3:
            n_blue = 3 * 6;
            n_red = 3 * 2;
            break;
        case 4:
            n_blue = 4 * 5;
            n_red = 4 * 3;
            break;
        case 5:
            n_blue = 4 * 5;
            n_red = 2 * 5 + 1;
            break;
        case 6:
            n_blue = 3 * 6;
            n_red = 2 * 6;
            break;
    }
    list<Tile*> tmp;
    tmp = get_tile_pointers(blue_tiles, mandatory_tile_numbers);
    n_blue -= tmp.size();
    random_tiles = tmp;

    tmp = get_tile_pointers(red_tiles, mandatory_tile_numbers);
    n_red -= tmp.size();
    random_tiles.insert(random_tiles.end(), tmp.begin(), tmp.end());

    for (auto s : get_shuffled_list(blue_tiles)) {
        if (find(random_tiles.begin(), random_tiles.end(), s) == random_tiles.end()) {
            random_tiles.push_back(s);
            n_blue--;
            if (not n_blue) {
                break;
            }
        }
    }

    for (auto s : get_shuffled_list(red_tiles)) {
        if (find(random_tiles.begin(), random_tiles.end(), s) == random_tiles.end()) {
            random_tiles.push_back(s);
            n_red--;
            if (not n_red) {
                break;
            }
        }
    }

    for (auto s : movable_systems) {
        printf("included tile - %d\n", s->get_number());
    }

    random_tiles = get_shuffled_list(random_tiles);
    movable_systems.clear();
    for (int i = 0; i < 7;i++) {
        for (int j = 0; j < 7; j++) {
            if (not grid[i][j]) {
                place_tile({i, j}, random_tiles.front());
                movable_systems.push_back(random_tiles.front());
                random_tiles.pop_front();
            }
        }
    }
}


void Galaxy::print_grid() {
    int I = 7;
    int J = 7;
    for (int i = 0; i < I; i++) {
        for (int i_pad = 0; i_pad < I - i; i_pad++) {
            cout << "  ";
        }
        for (int j = 0; j < J; j++) {
            if (grid[i][j] and grid[i][j] != &boundary_tile) {
                printf(" %02d ", grid[i][j]->get_number());
            } else {
                cout << "    ";
            }
        }
        cout << endl << endl;
    }
}

Tile* Galaxy::get_tile_at(Location l) {
    //TODO different size?
    if (l.i < 0 or l.i > 6 or l.j < 0 or l.j > 6) {
        return NULL;
    }
    Tile * tile = grid[l.i][l.j];
    if (tile == &boundary_tile) {
        return NULL;
    }
    return tile;
}

list<Tile*> Galaxy::get_adjacent(Tile *t1)
{
    list<Location> directions = {
        {0,1}, {1,1}, {1, 0}, {0, -1}, {-1, -1}, {-1, 0}
    };
    list<Tile*> adjacent;

    // Get tiles directly adjecent
    Location start_location = t1->get_location();
    for (auto it : directions) {
        Location new_location = start_location + it;
        Tile* potential_adjacent = get_tile_at(new_location);
        if (potential_adjacent) {
            adjacent.push_back(potential_adjacent);
        }
    }
    // Get connected wormholes
    if (t1->get_wormhole()) {
        for(Tile* it : wormhole_systems[t1->get_wormhole()]) {
            if (it != t1) {
                adjacent.push_back(it);
            }
        }
    }

    return adjacent;
};

struct VisitInfo {
    Tile* tile;
    float distance_to;
};

map<Tile*, float> Galaxy::distance_to_other_tiles(Tile* t1) {
    map<Tile*, float> visited;

    list<VisitInfo> to_visit;
    to_visit.push_back({t1, 0});

    while (to_visit.size()) {
        auto cur_tile = to_visit.front().tile;
        auto distance = to_visit.front().distance_to;
        to_visit.pop_front();

        // Skip home systems that are not the start system
        if (cur_tile->is_home_system() and distance > 0) {
            continue;
        }

        // If we didn't already visit the current tile or we got here via a
        // shorter distance, record the new distance and keep going
        if ((not visited.count(cur_tile)) or visited[cur_tile] > distance) {
            visited[cur_tile] = distance;

            // TODO: configure these?
            float move_cost;
            switch (cur_tile->get_anomaly()) {
                case NEBULA: move_cost = 2; break;
                case ASTEROID_FIELD: move_cost = 1.5; break;
                case SUPERNOVA: continue;
                case GRAVITY_RIFT: move_cost = 1; break; 
                default : move_cost = 1;
            }

            for (auto adjacent : get_adjacent(cur_tile)) {
                to_visit.push_back({adjacent, distance + move_cost});
            }
        }
    }
    
    return visited;
}


double_tile_map Galaxy::calculate_stakes(double_tile_map distances)
{
    double_tile_map stakes;
    for (auto t : movable_systems) {
        map<Tile*, float> stakes_in_system;
        for (auto hs : home_systems) {
            try {
                stakes_in_system[hs] = 1.0 / pow(distances[hs].at(t), 2);
            } catch (out_of_range) {
                // Systems like supernovas will not have any stake in them
                stakes_in_system[hs] = 0;
            }
        }
        float total_stake = 0;
        for (auto it : stakes_in_system) {
            total_stake += it.second;
        }
        if (total_stake == 0) {
            continue;
        }
        for (auto it : stakes_in_system) {
            stakes_in_system[it.first] /= total_stake;
        }
        stakes[t] = stakes_in_system;
    }
    return stakes;
}

typedef struct Scores
{
    map<Tile*, float> resource_share;
    map<Tile*, float> influence_share;
    map<Tile*, float> tech_share;
} Scores;

float average(list<float> l) 
{
    float sum = 0;
    for (auto it : l) {
        sum += it;
    }
    return sum / l.size();
}

float coefficient_of_variation(list<float> l) {
    float sum = 0;
    float avg = average(l);
    for (auto it : l) {
        sum += pow(it - avg, 2);
    }
    return sum / l.size() / avg;
}

/* Returns true if there exists a supernova tile with a distance to the muaat
 * home system less than or equal to near_dist
 */
bool Galaxy::is_supernova_near_muaat(int near_dist, double_tile_map distances)
{
    // Check to see if muaat is in this game
    int muaat_tile_number = 4;
    Tile* muaat_home_tile = NULL;
    for (auto hs : home_systems) {
        if (hs->get_number() == muaat_tile_number) {
            muaat_home_tile = hs;
        }
    }
    // No penalty if muaat are not in this game
    if (not muaat_home_tile) {
        return 0;
    }

    for (auto t : movable_systems) {
        if (t->get_anomaly() == SUPERNOVA) {
            if (distances[muaat_home_tile][t] <= near_dist) {
                return true;
            }
        }
    }
    return false;
}

/* Returns true if there exists a wormhole tile with a distane to the creuss
 * home system less than or equal to near_dist
 */
bool Galaxy::winnu_have_clear_path_to_mecatol(double_tile_map distances)
{
    // Check to see if Winnu is in this game
    int winnu_tile_number = 7;
    Tile* winnu_home_tile = NULL;
    for (auto hs : home_systems) {
        if (hs->get_number() == winnu_tile_number) {
            winnu_home_tile = hs;
        }
    }
    // No penalty if winnu are not in this game
    if (not winnu_home_tile) {
        return 0;
    }

    // TODO needs to change for non-standard board shapes if they ever get supported
    if (distances[winnu_home_tile][mecatol] <= 3) {
        return true;
    }
    return false;
}

/* Returns true if there is a clear path for winnu to mecatol
 */
bool Galaxy::is_wormhole_near_creuss(int near_dist, double_tile_map distances)
{
    // Check to see if winnu is in this game
    int creuss_tile_number = 17;
    Tile* creuss_home_tile = NULL;
    for (auto hs : home_systems) {
        if (hs->get_number() == creuss_tile_number) {
            creuss_home_tile = hs;
        }
    }
    // No penalty if creuss are not in this game
    if (not creuss_home_tile) {
        return 0;
    }

    for (auto t : movable_systems) {
        if (t->get_wormhole() and t->get_wormhole() != DELTA) {
            if (distances[creuss_home_tile][t] <= near_dist) {
                return true;
            }
        }
    }
    return false;
}

void Galaxy::set_evaluate_option(string name, float val)
{
    evaluate_options[name] = val;
}

void Galaxy::print_distances_from(int tile_num)
{
    Tile* home_tile = NULL;
    for (auto hs : home_systems) {
        if (hs->get_number() == tile_num) {
            home_tile = hs;
        }
    }
    if (not home_tile) {
        cout << "TIle not found" << endl;
    }

    map<Tile*, float> dists = distance_to_other_tiles(home_tile);
    cout << "Distance from tile " << home_tile->get_number() << " to tile:" << endl;
    for (auto dist : dists) {
        cout << "\t" << dist.first->get_number() << " " << dist.second << endl;
    }
}

int Galaxy::count_adjacent_anomalies()
{
    int count = 0;
    for (auto t : movable_systems) {
        if (t->get_anomaly() and t->get_anomaly() != EMPTY) {
            for (auto a : get_adjacent(t)) {
                if (a->get_anomaly() and t->get_anomaly() != EMPTY) {
                    count++;
                }
            }
        }
    }
    return count;
}

int Galaxy::count_adjacent_wormholes()
{
    int count = 0;
    for (auto t : movable_systems) {
        if (t->get_wormhole()) {
            for (auto a : get_adjacent(t)) {
                if (t->get_wormhole() == a->get_wormhole()) {
                    count++;
                }
            }
        }
    }
    return count;
}

float Galaxy::evaluate_grid() {
    
    double_tile_map distances_from_home_systems;

    for (auto home_system : home_systems) {
        distances_from_home_systems[home_system] = distance_to_other_tiles(home_system);
    }
    double_tile_map stakes = calculate_stakes(distances_from_home_systems);

    //for (auto it1 : stakes) {
    //    cout << "Stakes in " << it1.first->get_description_string() << endl;
    //    for (auto it2 : it1.second) {
    //        cout << "\t" << it2.second << " " << it2.first->get_description_string() << endl;
    //    }
    //}
  
   
    float score = 0;

    score += count_adjacent_anomalies();
    score += count_adjacent_wormholes() * 2;

    // Some race specific options if requested. the large score penalty ensures
    // that these will be satisfied if possible
    if (evaluate_options["muaat_gets_supernova"]) {
        if (not is_supernova_near_muaat(evaluate_options["muaat_gets_supernova"], 
                    distances_from_home_systems)) {
            score += 10;
        }
    }
    if (evaluate_options["creuss_gets_wormhole"]) {
        if (not is_wormhole_near_creuss(evaluate_options["creuss_gets_wormhole"], 
                    distances_from_home_systems)) {
            score += 10;
        }
    }
    if (evaluate_options["winnu_have_clear_path_to_mecatol"]) {
        if (not winnu_have_clear_path_to_mecatol( distances_from_home_systems)) {
            score += 10;
        }
    }

    Scores scores;

    float total_resources = 0;
    float total_influence = 0;
    list<float> resource_shares;
    list<float> influence_shares;
    list<float> combined_shares;
    list<float> tech_shares;
    for (auto home_system : home_systems) {
        float resource_share = 0;
        float influence_share = 0;
        float tech_share = 0;
        for (auto tile : movable_systems) {
            resource_share += tile->get_resource_value() * stakes[tile][home_system];
            influence_share += tile->get_influence_value() * stakes[tile][home_system];
            tech_share += tile->get_techcolor() ? stakes[tile][home_system] : 0;
        }
        scores.resource_share[home_system] = resource_share;
        scores.influence_share[home_system] = influence_share;
        scores.tech_share[home_system] = tech_share;

        resource_shares.push_back(resource_share);
        influence_shares.push_back(influence_share);
        tech_shares.push_back(tech_share);
        combined_shares.push_back(resource_share + influence_share);

        total_resources += resource_share;
        total_influence += influence_share;
    }

    //for (auto hs: home_systems) {
    //    printf("%2.1f %2.1f %s\n", scores.resource_share[hs], scores.influence_share[hs],
    //            hs->get_description_string().c_str());
    //}
    //printf("%2.1f %2.1f %s\n", total_resources, total_influence, "Totals");
    //printf("%0.3f %0.3f %03f %s\n", coefficient_of_variation(resource_shares), 
    //        coefficient_of_variation(influence_shares), 
    //        coefficient_of_variation(tech_shares), 
    //        "CVs");
    
    score += coefficient_of_variation(resource_shares) * evaluate_options["resource_weight"]
           + coefficient_of_variation(influence_shares) * evaluate_options["influence_weight"]
           + coefficient_of_variation(tech_shares) * evaluate_options["tech_weight"];

    return score;
}

void Galaxy::swap_tiles(Tile* a, Tile* b)
{
    Location a_start = a->get_location();
    Location b_start = b->get_location();

    place_tile(a_start, b);
    place_tile(b_start, a);
}

/* Returns every possible combination of swappable tiles
 */
vector<pair<Tile*, Tile*>> Galaxy::make_swap_list()
{
    vector<pair<Tile*, Tile*>> swap_list;
    for (auto t1 = movable_systems.begin(); t1 != movable_systems.end(); t1++) {
        auto t2 = t1;
        advance(t2, 1);
        for (; t2 != movable_systems.end(); t2++) {
            swap_list.push_back({*t1, *t2});
        }
    }
    random_shuffle(swap_list.begin(), swap_list.end());
    return swap_list;
}

/* optimize_grid
 * Swaps tiles to maximize the score of the current grid until no more swaps 
 * can be made. The score is defined by evaluate_grid
 */
void Galaxy::optimize_grid()
{
    float current_score = evaluate_grid();
    bool better_score_found = 0;

    int n_swaps = 0;
    // Set to a very high value to test all swaps
    int swaps_until_quit = 100000;

    do {
        better_score_found = false;
        auto swaps = make_swap_list();

        n_swaps = 0;
        for (auto swap : swaps) {
            swap_tiles(swap.first, swap.second);
            float new_score = evaluate_grid();
            if (new_score < current_score) {
                better_score_found = true;
                current_score = new_score;
                printf("Swapping tiles %d & %d, new_score: %0.3f\n", 
                        swap.first->get_number(), swap.second->get_number(), 
                        current_score);
                break;
            } else {
                swap_tiles(swap.first, swap.second);
            }
            n_swaps++;
            if (n_swaps > swaps_until_quit) {
                break;
            }
        }
    } while (better_score_found and n_swaps < swaps_until_quit);
}

void Galaxy::write_json(string filename)
{
    json j;
    j["grid"] = json::array();
    for (int i = 0; i < 7; i++) {
        auto row = json::array();
        for (int j = 0; j < 7; j++) {
            row.push_back(grid[i][j]->get_number());
        }
        j["grid"].push_back(row);
    }
    
    cerr << "Writing result to " << filename << endl;
    ofstream galaxy_output_file;
    galaxy_output_file.open(filename);
    galaxy_output_file << j;
    galaxy_output_file.close();
}

int main(int argc, char *argv[]) {

    cxxopts::Options options("ti4-map-generator", "Generate balanced TI4 maps");
    options.add_options()
            ("h,help", "Print help")
            ("t,tiles", "json file defining tile properites", cxxopts::value<std::string>())
            ("o,output", "galaxy json output filename", cxxopts::value<std::string>())
            ("p,players", "number of players", cxxopts::value<int>()->default_value("6"))
            ("s,seed", "random seed", cxxopts::value<int>())
            ("dummy_homes", "use blank home systems (default)")
            ("random_homes", "use random race home systems")
            ("choose_homes", "use with --races option")
            ("r,races", "list of home system tile numbers like so \"1 2 5...\"", cxxopts::value<string>()->default_value("6"))
            ("mandatory_tiles", "List of mandatory tiles to include", cxxopts::value<string>())
            ("creuss_gets_wormhole", "If creuss in game place a wormhole within x distance of it", cxxopts::value<int>()->default_value("1"))
            ("muaat_gets_supernova", "If muaat in game place the supernova within x distance of it", cxxopts::value<int>()->default_value("1"))
            ("winnu_have_clear_path_to_mecatol", "If winnu in game give them a clear path to mecatol", cxxopts::value<int>()->default_value("1"))
            ("resource_weight", "Relative weight of resource variance", cxxopts::value<float>()->default_value("1.0"))
            ("influence_weight", "Relative weight of infuence variance", cxxopts::value<float>()->default_value("1.0"))
            ("tech_weight", "Relative weight of tech specialty variance", cxxopts::value<float>()->default_value("1.0"))
            ;
    auto result = options.parse(argc, argv);

	if (result.count("help"))
    {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }

    if (not result.count("tiles") or not result.count("output")) {
        std::cerr << options.help({""}) << std::endl;
        exit(-1);
    }

    if (not result.count("seed")) {
        srand(time(NULL));
    } else {
        srand(result["seed"].as<int>());
    }

    HomeSystemSetups hss = DUMMY;
    string races;

    if (result.count("random_homes")) {
        hss = RANDOM_RACES;
    }
    if (result.count("choose_homes")) {
        hss = CHOSEN_RACES;
        if (not result.count("races")) {
            cerr << "Must also provide list of races with -r option" << endl;
            exit(-1);
        }
        races = result["races"].as<string>();
    }

    string mandatory_tiles = "";
    if (result.count("mandatory_tiles")) {
        mandatory_tiles = result["mandatory_tiles"].as<string>();
    }

    Galaxy galaxy(result["tiles"].as<string>(), result["players"].as<int>(), hss, races, mandatory_tiles);
    float score = galaxy.evaluate_grid();
    cout << "Score: " << score << endl;

    galaxy.set_evaluate_option("creuss_gets_wormhole", result["creuss_gets_wormhole"].as<int>());
    galaxy.set_evaluate_option("muaat_gets_supernova", result["muaat_gets_supernova"].as<int>());
    galaxy.set_evaluate_option("winnu_have_clear_path_to_mecatol", result["winnu_have_clear_path_to_mecatol"].as<int>());

    galaxy.set_evaluate_option("resource_weight", result["resource_weight"].as<float>());
    galaxy.set_evaluate_option("influence_weight", result["influence_weight"].as<float>());
    galaxy.set_evaluate_option("tech_weight", result["tech_weight"].as<float>());

    galaxy.optimize_grid();
    score = galaxy.evaluate_grid();
    cout << "Score: " << score << endl;
    galaxy.print_grid();
    galaxy.write_json(result["output"].as<string>());

    return 0;
}
