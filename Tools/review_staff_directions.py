"""Contact sheet from eight actual packaged-game casting captures."""
from pathlib import Path
from PIL import Image, ImageDraw

root = Path(__file__).resolve().parents[1]
sheet = Image.new('RGB', (1600, 660), '#15191b')
draw = ImageDraw.Draw(sheet)
for direction, label in enumerate(('N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW')):
    source = root / f'Saved/SpellStaffQA/D{direction}/Saved/IsoPrototype.png'
    frame = Image.open(source).convert('RGB').crop((330, 160, 1030, 690))
    frame.thumbnail((400, 305), Image.Resampling.LANCZOS)
    x, y = (direction % 4) * 400, (direction // 4) * 330
    sheet.paste(frame, (x, y + 22))
    draw.text((x + 12, y + 5), label, fill='white')
sheet.save(root / 'Saved/SpellStaffQA/Staff-directions.jpg', quality=94)
