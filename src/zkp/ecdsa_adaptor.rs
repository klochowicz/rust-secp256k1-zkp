//! # ECDSA Adaptor
//! Support for ECDSA based adaptor signatures.
//!
//! WARNING: ECDSA adaptor signatures are insecure when the secret key is reused
//! in certain other crypto schemes. See
//! https://github.com/ElementsProject/secp256k1-zkp/blob/6955af5ca8930aa674e5fdbc4343e722b25e0ca8/include/secp256k1_ecdsa_adaptor.h#L14
//! for details.
//!

use core::{fmt, ptr, str};
use ffi::{self, CPtr, ECDSA_ADAPTOR_SIGNATURE_LENGTH};
use {constants, PublicKey, Secp256k1, SecretKey};
use {from_hex, Error};
use {Message, Signing};
use {Signature, Verification};

/// Represents an adaptor signature and dleq proof.
#[derive(Debug, PartialEq, Clone, Copy, Eq)]
pub struct EcdsaAdaptorSignature(ffi::EcdsaAdaptorSignature);

impl fmt::LowerHex for EcdsaAdaptorSignature {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        for ch in self.0.as_bytes().iter() {
            write!(f, "{:02x}", ch)?;
        }
        Ok(())
    }
}

impl fmt::Display for EcdsaAdaptorSignature {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        fmt::LowerHex::fmt(self, f)
    }
}

impl str::FromStr for EcdsaAdaptorSignature {
    type Err = Error;
    fn from_str(s: &str) -> Result<EcdsaAdaptorSignature, Error> {
        let mut res = [0; ECDSA_ADAPTOR_SIGNATURE_LENGTH];
        match from_hex(s, &mut res) {
            Ok(ECDSA_ADAPTOR_SIGNATURE_LENGTH) => {
                EcdsaAdaptorSignature::from_slice(&res[0..ECDSA_ADAPTOR_SIGNATURE_LENGTH])
            }
            _ => Err(Error::InvalidEcdsaAdaptorSignature),
        }
    }
}

impl CPtr for EcdsaAdaptorSignature {
    type Target = ffi::EcdsaAdaptorSignature;
    fn as_c_ptr(&self) -> *const Self::Target {
        self.as_ptr()
    }

    fn as_mut_c_ptr(&mut self) -> *mut Self::Target {
        self.as_mut_ptr()
    }
}

impl EcdsaAdaptorSignature {
    /// Creates an [`EcdsaAdaptorSignature`] directly from a slice
    #[inline]
    pub fn from_slice(data: &[u8]) -> Result<EcdsaAdaptorSignature, Error> {
        match data.len() {
            ECDSA_ADAPTOR_SIGNATURE_LENGTH => {
                let mut ret = [0; ECDSA_ADAPTOR_SIGNATURE_LENGTH];
                ret[..].copy_from_slice(data);
                Ok(EcdsaAdaptorSignature(ffi::EcdsaAdaptorSignature::from(ret)))
            }
            _ => Err(Error::InvalidEcdsaAdaptorSignature),
        }
    }

    /// Obtains a raw const pointer suitable for use with FFI functions
    #[inline]
    pub fn as_ptr(&self) -> *const ffi::EcdsaAdaptorSignature {
        &self.0
    }

    /// Obtains a raw mutable pointer suitable for use with FFI functions
    #[inline]
    pub fn as_mut_ptr(&mut self) -> *mut ffi::EcdsaAdaptorSignature {
        &mut self.0
    }
}

impl EcdsaAdaptorSignature {
    /// Creates an adaptor signature along with a proof to verify the adaptor signature.
    pub fn encrypt<C: Signing>(
        secp: &Secp256k1<C>,
        msg: &Message,
        sk: &SecretKey,
        enckey: &PublicKey,
    ) -> EcdsaAdaptorSignature {
        let mut adaptor_sig = ffi::EcdsaAdaptorSignature::new();

        unsafe {
            debug_assert!(
                ffi::secp256k1_ecdsa_adaptor_encrypt(
                    *secp.ctx(),
                    &mut adaptor_sig,
                    sk.as_c_ptr(),
                    enckey.as_c_ptr(),
                    msg.as_c_ptr(),
                    None,
                    ptr::null_mut(),
                ) == 1
            );
        };

        EcdsaAdaptorSignature(adaptor_sig)
    }

    /// Creates an ECDSA signature from an adaptor signature and an adaptor secret.
    pub fn decrypt(&self, decryption_key: &SecretKey) -> Result<Signature, Error> {
        unsafe {
            let mut signature = ffi::Signature::new();
            let ret = ffi::secp256k1_ecdsa_adaptor_decrypt(
                ffi::secp256k1_context_no_precomp,
                &mut signature,
                decryption_key.as_c_ptr(),
                self.as_c_ptr(),
            );

            if ret != 1 {
                return Err(Error::CannotDecryptAdaptorSignature);
            }

            Ok(Signature::from(signature))
        }
    }

    /// Extracts the adaptor secret from the complete signature and the adaptor signature.
    pub fn recover<C: Signing>(
        &self,
        secp: &Secp256k1<C>,
        sig: &Signature,
        encryption_key: &PublicKey,
    ) -> Result<SecretKey, Error> {
        let mut data: [u8; constants::SECRET_KEY_SIZE] = [0; constants::SECRET_KEY_SIZE];

        let ret = unsafe {
            ffi::secp256k1_ecdsa_adaptor_recover(
                *secp.ctx(),
                data.as_mut_c_ptr(),
                sig.as_c_ptr(),
                self.as_c_ptr(),
                encryption_key.as_c_ptr(),
            )
        };

        if ret != 1 {
            return Err(Error::CannotRecoverAdaptorSecret);
        }

        Ok(SecretKey::from_slice(&data)?)
    }

    /// Verifies that the adaptor secret can be extracted from the adaptor signature and the completed ECDSA signature.
    pub fn verify<C: Verification>(
        &self,
        secp: &Secp256k1<C>,
        msg: &Message,
        pubkey: &PublicKey,
        encryption_key: &PublicKey,
    ) -> Result<(), Error> {
        let res = unsafe {
            ffi::secp256k1_ecdsa_adaptor_verify(
                *secp.ctx(),
                self.as_c_ptr(),
                pubkey.as_c_ptr(),
                msg.as_c_ptr(),
                encryption_key.as_c_ptr(),
            )
        };

        if res != 1 {
            return Err(Error::CannotVerifyAdaptorSignature);
        };

        Ok(())
    }
}

#[cfg(all(test, feature = "global-context"))]
mod tests {
    use super::Message;
    use super::*;
    use rand::thread_rng;
    use SECP256K1;

    #[test]
    #[cfg(not(rust_secp_fuzz))]
    fn test_ecdsa_adaptor_signature() {
        let (seckey, pubkey) = SECP256K1.generate_keypair(&mut thread_rng());
        let (adaptor_secret, adaptor) = SECP256K1.generate_keypair(&mut thread_rng());
        let msg = Message::from_slice(&[2u8; 32]).unwrap();
        let adaptor_sig = EcdsaAdaptorSignature::encrypt(&SECP256K1, &msg, &seckey, &adaptor);

        adaptor_sig
            .verify(&SECP256K1, &msg, &pubkey, &adaptor)
            .expect("adaptor signature to be valid");
        adaptor_sig
            .verify(&SECP256K1, &msg, &adaptor, &pubkey)
            .expect_err("adaptor signature to be invalid");
        let sig = adaptor_sig
            .decrypt(&adaptor_secret)
            .expect("to be able to decrypt using the correct secret");
        SECP256K1
            .verify(&msg, &sig, &pubkey)
            .expect("signature to be valid");
        let recovered = adaptor_sig
            .recover(&SECP256K1, &sig, &adaptor)
            .expect("to be able to recover the secret");
        assert_eq!(adaptor_secret, recovered);
    }

    #[test]
    fn test_ecdsa_adaptor_signature_plain_valid() {
        let msg = msg_from_str("8131e6f4b45754f2c90bd06688ceeabc0c45055460729928b4eecf11026a9e2d");
        let pubkey = "035be5e9478209674a96e60f1f037f6176540fd001fa1d64694770c56a7709c42c"
            .parse()
            .unwrap();
        let encryption_key = "02c2662c97488b07b6e819124b8989849206334a4c2fbdf691f7b34d2b16e9c293"
            .parse()
            .unwrap();
        let adaptor_sig : EcdsaAdaptorSignature = "03424d14a5471c048ab87b3b83f6085d125d5864249ae4297a57c84e74710bb6730223f325042fce535d040fee52ec13231bf709ccd84233c6944b90317e62528b2527dff9d659a96db4c99f9750168308633c1867b70f3a18fb0f4539a1aecedcd1fc0148fc22f36b6303083ece3f872b18e35d368b3958efe5fb081f7716736ccb598d269aa3084d57e1855e1ea9a45efc10463bbf32ae378029f5763ceb40173f"
            .parse()
            .unwrap();

        adaptor_sig
            .verify(&SECP256K1, &msg, &pubkey, &encryption_key)
            .expect("adaptor signature verification to pass");

        let sig = compact_sig_from_str("424d14a5471c048ab87b3b83f6085d125d5864249ae4297a57c84e74710bb67329e80e0ee60e57af3e625bbae1672b1ecaa58effe613426b024fa1621d903394");
        let expected_decryption_key: SecretKey =
            "0b2aba63b885a0f0e96fa0f303920c7fb7431ddfa94376ad94d969fbf4109dc8"
                .parse()
                .unwrap();

        let recovered = adaptor_sig
            .recover(&SECP256K1, &sig, &encryption_key)
            .expect("to be able to recover the decryption key");

        assert_eq!(expected_decryption_key, recovered);
    }

    #[test]
    fn test_ecdsa_adaptor_signature_wrong_proof() {
        let msg = msg_from_str("8131e6f4b45754f2c90bd06688ceeabc0c45055460729928b4eecf11026a9e2d");
        let pubkey = "035be5e9478209674a96e60f1f037f6176540fd001fa1d64694770c56a7709c42c"
            .parse()
            .unwrap();
        let encryption_key = "0214ccb756249ad6e733c80285ea7ac2ee12ffebbcee4e556e6810793a60c45ad4"
            .parse()
            .unwrap();
        let adaptor_sig: EcdsaAdaptorSignature = "03f94dca206d7582c015fb9bffe4e43b14591b30ef7d2b464d103ec5e116595dba03127f8ac3533d249280332474339000922eb6a58e3b9bf4fc7e01e4b4df2b7a4100a1e089f16e5d70bb89f961516f1de0684cc79db978495df2f399b0d01ed7240fa6e3252aedb58bdc6b5877b0c602628a235dd1ccaebdddcbe96198c0c21bead7b05f423b673d14d206fa1507b2dbe2722af792b8c266fc25a2d901d7e2c335"
            .parse()
            .unwrap();

        adaptor_sig
            .verify(&SECP256K1, &msg, &pubkey, &encryption_key)
            .expect_err("providing a wrong proof should fail validation");
    }

    #[test]
    fn test_ecdsa_adaptor_signature_recover_wrong_sig_r_value() {
        let encryption_key = "035176d24129741b0fcaa5fd6750727ce30860447e0a92c9ebebdeb7c3f93995ed"
            .parse()
            .unwrap();
        let adaptor_sig: EcdsaAdaptorSignature = "03aa86d78059a91059c29ec1a757c4dc029ff636a1e6c1142fefe1e9d7339617c003a8153e50c0c8574a38d389e61bbb0b5815169e060924e4b5f2e78ff13aa7ad858e0c27c4b9eed9d60521b3f54ff83ca4774be5fb3a680f820a35e8840f4aaf2de88e7c5cff38a37b78725904ef97bb82341328d55987019bd38ae1745e3efe0f8ea8bdfede0d378fc1f96e944a7505249f41e93781509ee0bade77290d39cd12"
            .parse()
            .unwrap();

        let sig = compact_sig_from_str("f7f7fe6bd056fc4abd70d335f72d0aa1e8406bba68f3e579e4789475323564a452c46176c7fb40aa37d5651341f55697dab27d84a213b30c93011a7790bace8c");
        adaptor_sig
            .recover(&SECP256K1, &sig, &encryption_key)
            .expect_err("providing wrong r value should prevent us from recovering decryption key");
    }

    #[test]
    fn test_ecdsa_adaptor_signature_recover_from_high_s_signature() {
        let encryption_key = "02042537e913ad74c4bbd8da9607ad3b9cb297d08e014afc51133083f1bd687a62"
            .parse()
            .unwrap();
        let adaptor_sig: EcdsaAdaptorSignature = "032c637cd797dd8c2ce261907ed43e82d6d1a48cbabbbece801133dd8d70a01b1403eb615a3e59b1cbbf4f87acaf645be1eda32a066611f35dd5557802802b14b19c81c04c3fefac5783b2077bd43fa0a39ab8a64d4d78332a5d621ea23eca46bc011011ab82dda6deb85699f508744d70d4134bea03f784d285b5c6c15a56e4e1fab4bc356abbdebb3b8fe1e55e6dd6d2a9ea457e91b2e6642fae69f9dbb5258854"
            .parse()
            .unwrap();

        let sig = compact_sig_from_str("2c637cd797dd8c2ce261907ed43e82d6d1a48cbabbbece801133dd8d70a01b14b5f24321f550b7b9dd06ee4fcfd82bdad8b142ff93a790cc4d9f7962b38c6a3b");
        let expected_decryption_key: SecretKey =
            "324719b51ff2474c9438eb76494b0dc0bcceeb529f0a5428fd198ad8f886e99c"
                .parse()
                .unwrap();
        let recovered = adaptor_sig
            .recover(&SECP256K1, &sig, &encryption_key)
            .expect("with high s we should still be able to recover the decryption key");

        assert_eq!(expected_decryption_key, recovered);
    }

    fn msg_from_str(input: &str) -> Message {
        let mut buf = [0u8; 32];
        from_hex(input, &mut buf).unwrap();
        Message::from_slice(&buf).unwrap()
    }

    fn compact_sig_from_str(input: &str) -> Signature {
        let mut buf = [0u8; 64];
        from_hex(input, &mut buf).unwrap();
        Signature::from_compact(&buf).unwrap()
    }
}
