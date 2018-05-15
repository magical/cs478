extern crate getopts;
use std::io::Write;
use std::os::raw::c_ulong;

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let program = args[0].clone();

    let mut opts = getopts::Options::new();
    opts.optflag("h", "help", "display help");
    opts.optflag("d", "dec", "decimal output (default)");
    opts.optflag("b", "bin", "binary/raw output");
    opts.optflag("x", "hex", "hex output");
    let matches = opts.parse(&args[1..]).unwrap();

    if matches.opt_present("h") {
        println!("{}", opts.usage(&format!("Usage: {} [options]", program)));
        return;
    }

    let random = match auxv_random() {
        None => { panic!("couldn't get auxv random") },
        Some(x) => x,
    };

    if matches.opt_present("b") {
        std::io::stdout().write_all(&random[..]).unwrap();
    } else if matches.opt_present("x") {
        for x in random.iter() {
            print!("{:02x}", x);
        }
        println!("");
    } else {
        for (i, x) in random.iter().enumerate() {
            if i != 0 {
                print!(" ")
            }
            print!("{}", x)
        }
        //let out: String = random.iter().map(|x| format!("{}", x)).collect::<Vec<_>>().join(" ");
        //println!("{}", out);
    }
}

extern "C" {
    fn getauxval(ty: c_ulong) -> c_ulong;
}

static AT_RANDOM: c_ulong = 25;

fn auxv_random() -> Option<&'static [u8; 16]> {
    unsafe {
        let r = getauxval(AT_RANDOM);
        if r == 0 {
            None
        } else {
            Some(std::mem::transmute(r))
        }
    }
}
