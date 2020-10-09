# get-otp

This repository holds a combination of tools that can be used to (comfortably)
generate TOTP 2FA access tokens on the desktop, instead of depending on a phone
app.

## get-otp

Main tool, depends on:

- [jq](https://stedolan.github.io/jq/)
- [fzf](https://github.com/junegunn/fzf)
- [OATH Toolkit](https://www.nongnu.org/oath-toolkit/)
- [wl-clipboard](https://github.com/bugaevc/wl-clipboard) (optional)

It will use the `cbc-file` executable from this project to decrypt a
`~/.local/share/otp_accounts` file, whose decrypted contents should be in the
same format as exported by [andOTP](https://github.com/andOTP/andOTP). It can
then run a menu, using `fzf`, to let you choose the account for which you want
an access token. If running on Wayland, the token will also be copied to the
clipboard.

```
$ get-otp [account_name]
```

## encrypt-otp

This tool doesn't depend on anything besides `cbc-file`. What it does is encrypt
the file passed to it (which should be in the format exported by andOTP, as
mentioned above) and put the encrypted file in the correct place.

```
$ encrypt-otp otp_accounts.json
```

## cbc-file

Hidden utility, does the encryption magic. Despite the name, uses
[ChaCha20+Poly1305](https://tools.ietf.org/html/rfc7539) for encryption, as
implemented by [BearSSL](https://www.bearssl.org/), together with
[argon2](https://github.com/p-h-c/phc-winner-argon2) for key derivation.

Both of the mentioned libraries are necessary for building this utility. On
Linux, a kernel which implements the
[getrandom(2)](https://man.voidlinux.org/getrandom.2) syscall is necessary,
since it is the backend for
[getentropy(3)](https://man.voidlinux.org/getentropy.3), which is the only
random number backend implemented.

## Disclaimer

This is experimental code and ideas, and shouldn't be put anywhere near any sort
of production. However, I am open to suggestions and improvements, so feel free
to reach out.
