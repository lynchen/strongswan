/*
 * Copyright (C) 2016 Andreas Steffen
 * HSR Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the Licenseor (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be usefulbut
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <crypto/crypto_tester.h>

/**
 * ChaCha20 Stream Test Vector from RFC 7539, Section 2.3.2
 */
xof_test_vector_t chacha20_xof_1 = {
	.alg = XOF_CHACHA20, .len = 44,
	.seed	= "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
			  "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
			  "\x00\x00\x00\x09\x00\x00\x00\x4a\x00\x00\x00\x00",
	.out_len = 64,
	.out	= "\x10\xf1\xe7\xe4\xd1\x3b\x59\x15\x50\x0f\xdd\x1f\xa3\x20\x71\xc4"
			  "\xc7\xd1\xf4\xc7\x33\xc0\x68\x03\x04\x22\xaa\x9a\xc3\xd4\x6c\x4e"
			  "\xd2\x82\x64\x46\x07\x9f\xaa\x09\x14\xc2\xd7\x05\xd9\x8b\x02\xa2"
			  "\xb5\x12\x9c\xd1\xde\x16\x4e\xb9\xcb\xd0\x83\xe8\xa2\x50\x3c\x4e"
};

/**
 * ChaCha20 Stream Test Vector from RFC 7539, Section 2.4.2
 */
xof_test_vector_t chacha20_xof_2 = {
	.alg = XOF_CHACHA20, .len = 44,
	.seed	= "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
			  "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
			  "\x00\x00\x00\x00\x00\x00\x00\x4a\x00\x00\x00\x00",
	.out_len = 114,
	.out	= "\x22\x4f\x51\xf3\x40\x1b\xd9\xe1\x2f\xde\x27\x6f\xb8\x63\x1d\xed"
			  "\x8c\x13\x1f\x82\x3d\x2c\x06\xe2\x7e\x4f\xca\xec\x9e\xf3\xcf\x78"
			  "\x8a\x3b\x0a\xa3\x72\x60\x0a\x92\xb5\x79\x74\xcd\xed\x2b\x93\x34"
			  "\x79\x4c\xba\x40\xc6\x3e\x34\xcd\xea\x21\x2c\x4c\xf0\x7d\x41\xb7"
			  "\x69\xa6\x74\x9f\x3f\x63\x0f\x41\x22\xca\xfe\x28\xec\x4d\xc4\x7e"
			  "\x26\xd4\x34\x6d\x70\xb9\x8c\x73\xf3\xe9\xc5\x3a\xc4\x0c\x59\x45"
			  "\x39\x8b\x6e\xda\x1a\x83\x2c\x89\xc1\x67\xea\xcd\x90\x1d\x7e\x2b"
			  "\xf3\x63"
};

/**
 * ChaCha20 Stream Test Vector #2 from RFC 7539, Section A1.
 */
xof_test_vector_t chacha20_xof_3 = {
	.alg = XOF_CHACHA20, .len = 44,
	.seed	= "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
			  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
			  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
	.out_len = 64,
	.out	= "\x9f\x07\xe7\xbe\x55\x51\x38\x7a\x98\xba\x97\x7c\x73\x2d\x08\x0d"
			  "\xcb\x0f\x29\xa0\x48\xe3\x65\x69\x12\xc6\x53\x3e\x32\xee\x7a\xed"
			  "\x29\xb7\x21\x76\x9c\xe6\x4e\x43\xd5\x71\x33\xb0\x74\xd8\x39\xd5"
			  "\x31\xed\x1f\x28\x51\x0a\xfb\x45\xac\xe1\x0a\x1f\x4b\x79\x4d\x6f"
};

/**
 * ChaCha20 Stream Test Vector #3 from RFC 7539, Section A1.
 */
xof_test_vector_t chacha20_xof_4 = {
	.alg = XOF_CHACHA20, .len = 44,
	.seed	= "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
			  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01"
			  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
	.out_len = 64,
	.out	= "\x3a\xeb\x52\x24\xec\xf8\x49\x92\x9b\x9d\x82\x8d\xb1\xce\xd4\xdd"
			  "\x83\x20\x25\xe8\x01\x8b\x81\x60\xb8\x22\x84\xf3\xc9\x49\xaa\x5a"
			  "\x8e\xca\x00\xbb\xb4\xa7\x3b\xda\xd1\x92\xb5\xc4\x2f\x73\xf2\xfd"
			  "\x4e\x27\x36\x44\xc8\xb3\x61\x25\xa6\x4a\xdd\xeb\x00\x6c\x13\xa0"
};

