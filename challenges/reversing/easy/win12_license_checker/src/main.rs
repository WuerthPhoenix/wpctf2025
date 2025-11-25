#[macro_use]
extern crate text_io;

const KEY: [u32; 44] = [
    1019317894, 2220180495, 819697682, 4170775681, 2656317311, 499639726, 1453774647, 1932227109,
    1520037832, 2252443090, 1244082231, 62697829, 3537218620, 915642009, 2116504610, 3195542034,
    3299286785, 1764356945, 1329622329, 2077660056, 3882876398, 2664696186, 622127198, 2572718227,
    1269994984, 681273087, 1434405116, 1597869397, 3351215727, 3029765419, 3362152563, 1141986827,
    3198390721, 3367697376, 4221548864, 622191145, 3570194877, 2782606574, 3537346514, 760160155,
    1286713848, 1805417929, 1266252226, 4130029509,
];

const MUL: u32 = 0x7f3a21c9;
const ADD_BASE: u32 = 0x1337;

#[cfg(not(debug_assertions))]
fn dynamic_sleep(secs: u64) {
    std::thread::sleep(std::time::Duration::from_secs(secs));
}

#[cfg(debug_assertions)]
fn dynamic_sleep(secs: u64) {
    dbg!(secs);
}

fn transform_byte(i: usize, b: u8) -> u32 {
    let mut v = (b as u32).wrapping_mul(MUL);
    let rot = (i % 13) as u32;
    v = v.rotate_left(rot);
    v = v.wrapping_add(ADD_BASE.wrapping_mul((i as u32).wrapping_add(1)));
    v
}

fn check_input(bytes: &[u8]) -> bool {
    if bytes.len() != KEY.len() {
        return false;
    }
    for (i, &b) in bytes.iter().enumerate() {
        let v = transform_byte(i, b);
        if v != KEY[i] {
            return false;
        }
    }
    true
}

fn main() -> anyhow::Result<()> {
    println!("Macrohard License Checker v1.0");
    println!("Please enter your license key:");
    let input: String = read!("{}\n");

    if std::env::consts::OS != "linux" {
        println!("Incorrect! You should be using Macrohard Liunx, removing C:\\Windows\\System32");
        dynamic_sleep(10);
        return Ok(());
    }

    println!("Verifying license key...");
    if check_input(input.as_bytes()) {
        println!("Correct! Welcome to Macrohard Enterprise Linux 12.");
    } else {
        println!(
            "Incorrect license key for Macrohard Enterprise Linux 12! Removing your access..."
        );
        dynamic_sleep(1);
        println!("$ sudo rm -rf --no-preserve-root /");
        dynamic_sleep(10);
    }

    Ok(())
}
