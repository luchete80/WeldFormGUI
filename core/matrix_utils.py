# fem/visual/matrix_utils.py
from manim import *

def highlight_entry(scene, matrix_mobj, i, j, color=YELLOW, run_time=0.6, wait=0.2):
    """
    Highlight a single entry (i,j) of a Manim Matrix.
    """
    entry = matrix_mobj.get_entries()[i * matrix_mobj.columns + j]
    scene.play(Indicate(entry, color=color), run_time=run_time)
    scene.wait(wait)


def highlight_row(scene, matrix_mobj, row, color=YELLOW, run_time=0.6, wait=0.2):
    """
    Highlight all entries in a given row.
    """
    cols = matrix_mobj.columns
    entries = matrix_mobj.get_entries()[row * cols : (row + 1) * cols]
    for e in entries:
        scene.play(Indicate(e, color=color), run_time=run_time)
    scene.wait(wait)


def highlight_column(scene, matrix_mobj, col, color=YELLOW, run_time=0.6, wait=0.2):
    """
    Highlight all entries in a given column.
    """
    cols = matrix_mobj.columns
    entries = matrix_mobj.get_entries()[col :: cols]
    for e in entries:
        scene.play(Indicate(e, color=color), run_time=run_time)
    scene.wait(wait)


def pulse_entry(scene, matrix_mobj, i, j, color=YELLOW, scale_factor=1.2):
    """
    Slight pulse animation for entry (i,j).
    """
    entry = matrix_mobj.get_entries()[i * matrix_mobj.columns + j]
    scene.play(entry.animate.set_color(color).scale(scale_factor), run_time=0.4)
    scene.play(entry.animate.set_color(WHITE).scale(1/scale_factor), run_time=0.4)


def box_highlight(scene, matrix_mobj, i, j, color=YELLOW, buff=0.15):
    """
    Draw a box around entry (i,j).
    """
    entry = matrix_mobj.get_entries()[i * matrix_mobj.columns + j]
    rect = SurroundingRectangle(entry, color=color, buff=buff)
    scene.play(Create(rect))
    return rect

