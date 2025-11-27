#include "database.h"

Database::Database() {
    hierarchy = new Hierarchy();
    hierarchy->isDatabase = true;
    Mission = new Hierarchy();
}
