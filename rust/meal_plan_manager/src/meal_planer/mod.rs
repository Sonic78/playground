use std::fs::File;

pub struct Meal {
    pub name : String,
    pub description: String,
    pub recipe : String,
    pub cooked_counter : u32,
}

pub fn load_meals_from_db(file: File) {

}

impl Meal {
    pub fn new(name : String, description: String, recipe : String) -> Meal {
        return Meal {name, description, recipe, cooked_counter: 0 };
    }
}