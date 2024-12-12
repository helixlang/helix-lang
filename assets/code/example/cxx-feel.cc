#include <iostream>
#include <string>
#include <utility>

template <typename T>
class Sound {
  protected:
    T value;

  public:
    explicit Sound(T value)
        : value(std::move(value)) {}

    void play() {
        std::cout << "Playing sound: "  << value << std::endl;
    }

    void stop() {
        std::cout << "Stopping sound: " << value << std::endl;
    }
};

class WoodenSpeaker : public Sound<std::string> {
  public:
    explicit WoodenSpeaker(std::string value)
        : Sound(std::move(value)) {}

    void play() {
        std::cout << "Playing sound on wooden speaker: " << this->value << std::endl;
    }
};

int main() {
    Sound<std::string> sound("beep");
    sound.play(); // Playing sound: beep
    sound.stop(); // Stopping sound: beep

    WoodenSpeaker woodenSpeaker("beep");
    woodenSpeaker.play(); // Playing sound on wooden speaker: beep
    woodenSpeaker.stop(); // Stopping sound: beep

    return 0;
}