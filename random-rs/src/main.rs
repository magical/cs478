extern crate libc;
extern crate getopts;
use std::io::Write;
fn main() {
    let args: Vec<String> = std::env::args().collect();
    let program = args[0].clone();

    let mut opts = getopts::Options::new();
    opts.optflag("h", "help", "display help");
    opts.optflag("d", "dec", "decimal output (default)");
    opts.optflag("b", "bin", "binary/raw output");
    opts.optflag("x", "hex", "hex output");
    let matches = opts.parse(&args[1..]).unwrap();

    let random: [u8; 16] = [0; 16];

    if matches.opt_present("h") {
        println!("{}", opts.usage(&format!("Usage: {} [options]", program)));
    } else if matches.opt_present("b") {
        std::io::stdout().write_all(&random[..]).unwrap();
    } else if matches.opt_present("x") {
        for x in random.iter() {
            print!("{:02x}", x);
        }
        println!("")
    } else {
        for x in random.iter() {
            print!("{} ", x);
        }
    }
}
