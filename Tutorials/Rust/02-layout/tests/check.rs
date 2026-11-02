// Runs the step's own `--test` mode, so `cargo test` checks the GUI logic with
// no window and no GPU.

#[test]
fn checks() {
    let status = std::process::Command::new(env!("CARGO_BIN_EXE_02-layout"))
        .arg("--test")
        .status()
        .expect("run the tutorial");
    assert!(status.success(), "the program reported a failure");
}
