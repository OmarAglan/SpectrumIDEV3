# Qalam visual identity

The Qalam mark is an Arabic `ق` shaped as a fountain-pen nib. Its two dots are
kept visible at small sizes, and the palette follows Qalam's existing blue
accent family. The mark contains no Latin text.

## Files

- `qalam/resources/branding/QalamLogo-final-v2.png`: transparent master.
- `qalam/resources/branding/icons/`: PNG application sizes from 16 to 512 px.
- `qalam/resources/QalamLogo.png`: Qt application/welcome logo.
- `qalam/resources/QalamLogo.ico`: Windows multi-size icon.
- `qalam/resources/QalamLogo.icns`: macOS icon family.

## Generation

The master concept was generated with the built-in image generation tool using
the `logo-brand` workflow. It requested a recognizable Arabic `ق`, a subtle pen
nib, Qalam blue/cyan, no Latin characters, and a flat green chroma background.
The installed image-skill chroma helper produced the transparent master; Pillow
then generated deterministic platform sizes from that one master.

The chroma source is retained for provenance. Do not resize it independently;
all shipping files derive from the transparent final master.
