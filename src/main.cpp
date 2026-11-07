#include "ExcalCore/Application.hpp"
#include "ExcalCore/ApplicationSpec.hpp"
#include "ExcalCore/Input/InputBindings.hpp"
#include "ExcalCore/Input/KeyCode.hpp"
#include "ExcalCore/Utils/Logging.hpp"

int main(void) {
  EE::ApplicationSpec spec = {.name = "Carduellum", .runOutOfFocus = false};
  EE::Application app(spec);

  auto& input = app.GetInput();
  input.BindInput<EE::BindingButton>("Accept", EE::KeyCode::RETURN);
  input.BindInput<EE::BindingAxis2D>(
    "Menu_Navigation", {EE::KeyCode::W, EE::KeyCode::S, EE::KeyCode::A, EE::KeyCode::D}
  );
  input.BindInput<EE::BindingButton>("Cancel", EE::KeyCode::BACKSPACE);
  input.BindInput<EE::BindingButton>("Pause", EE::KeyCode::ESCAPE);

  app.Run();

  return 0;
}
