// Appended to the existing native-animation-cpu-probe loader. No engine/window.
template<class T> void write(std::ofstream &f, const T &value) {
    f.write(reinterpret_cast<const char *>(&value), sizeof(value));
}
int main(int argc, char **argv) try {
    if (argc != 3) return 64;
    std::ofstream out(argv[2], std::ios::binary);
    std::vector<std::filesystem::path> files;
    for (const auto &entry : std::filesystem::directory_iterator(argv[1]))
        if (entry.path().extension() == ".an" && entry.path().stem().string().starts_with("Unarmed_"))
            files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    for (const auto &path : files) {
        const auto name = path.stem().string().substr(8);
        auto *info = load(path, name.c_str());
        const int bones = info->NumBones();
        const int frames = info->GetAniNumFrames();
        auto *action = info->AddAction("inspect", 1, frames-1);
        action->SetLoop(false);
        auto *animation = new AnimationImp(0, info);
        auto &player = animation->Player(0);
        player.SetAction("inspect"); player.Play(); player.Pause(); player.SetAutoStop(false);
        const uint32_t length = static_cast<uint32_t>(name.size());
        write(out, length); out.write(name.data(), length);
        write(out, frames); write(out, bones);
        const int samples = 4*(frames-2)+1;
        write(out, samples);
        for (int step = 0; step < samples; ++step) {
            player.SetPosition(float(step)/float(samples-1));
            animation->EvaluatePose();
            for (int bone = 0; bone < bones; ++bone) {
                const auto p = info->GetBone(bone).matrix.Pos();
                write(out, p.x); write(out, p.y); write(out, p.z);
            }
        }
    }
    return out ? 0 : 1;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n'; return 1;
}
