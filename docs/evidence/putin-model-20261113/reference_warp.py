"""Align a photographic face to the 512px GM render for an offline UV probe."""
from pathlib import Path
import numpy as np
import sys
from PIL import Image, ImageFilter

here = Path(__file__).resolve().parent
photo = Image.open(here/'reference-putin-2024.png').convert('RGB')
photo.thumbnail((512,512), Image.Resampling.LANCZOS)
assert photo.size == (364,512)
# Eye centres: photo (143,155)/(215,160), GM render (211,185)/(295,190).
# Mouth: photo ~(177,248), GM ~(251,281). This is a diagnostic affine fit,
# not a claim that a flat photo supplies correct profile geometry.
scale_x = 84/72
scale_y = 96/93
matrix = (1/scale_x, 0, 143-211/scale_x, 0, 1/scale_y, 155-185/scale_y)
aligned = photo.transform((512,512),Image.Transform.AFFINE,matrix,resample=Image.Resampling.BICUBIC)
aligned.save(here/'reference-aligned.png')
# Transfer only local color and detail: preserve the render's broad illumination.
model = sys.argv[1] if len(sys.argv)>1 else 'officer_5'
baseline = Image.open(here/f'{model}-candidate-preview.png').convert('RGB')
detail = np.asarray(aligned,dtype=float)-np.asarray(aligned.filter(ImageFilter.GaussianBlur(20)),dtype=float)
light = np.asarray(baseline.filter(ImageFilter.GaussianBlur(20)),dtype=float)
Image.fromarray(np.clip(light+detail,0,255).astype('uint8')).save(here/f'{model}-reference-normalized.png')
