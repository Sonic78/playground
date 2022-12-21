use meal_plan_manager::meal_planer::Meal;

#[cfg(test)]
mod tests {
    #[test]
    fn create_meal_works() {
        let sut = meal_plan_manager::meal_planer::Meal::new("MyMeal1".to_string(), "Descr".to_string(),  "Test".to_string());
        assert_eq!(0, sut.cooked_counter);
        assert_eq!("MyMeal1", sut.name);
        assert_eq!("Descr", sut.description);
        assert_eq!("Test", sut.recipe)
    }
}
