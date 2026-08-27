import tkinter as tk
from tkinter import filedialog, messagebox, ttk
import subprocess
from pathlib import Path
import ctypes
import threading
import os
import sys
import string
import shutil
import re

# --- JETBRAINS DARCKULA COLORS ---
JB_BG = "#1E1F22"
JB_FIELD = "#2B2D30"
JB_BORDER = "#393B40"
JB_TEXT = "#DFE1E5"
JB_BLUE = "#3574F0"
JB_COMMENT = "#6A9955"

def resource_path(relative_path):
    """ Get absolute path to resource, works for dev and for PyInstaller """
    try:
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")
    return os.path.join(base_path, relative_path)

class PDFBinConverter(tk.Frame):
    def __init__(self, master=None):
        super().__init__(master)
        self.master = master
        self.master.title("PDFtoBin")
        self.master.geometry("600x580") 
        self.master.configure(bg=JB_BG)
        
        self.selected_path = None
        self.cpp_exe = resource_path(os.path.join("build", "PDFtoBin.exe"))
        self.usb_mode = tk.BooleanVar(value=False)
        self.actual_usb_path = ""
        
        self.style = ttk.Style()
        self.style.theme_use('default')
        self.style.configure("JB.Horizontal.TProgressbar", 
                           troughcolor=JB_FIELD, 
                           background=JB_BLUE, 
                           thickness=6, 
                           bordercolor=JB_BORDER)
        
        self.setup_ui()

    def setup_ui(self):
        self.main_content = tk.Frame(self.master, bg=JB_BG, padx=40, pady=20)
        self.main_content.pack(fill="both", expand=True)

        # 1. Source Selection
        tk.Label(self.main_content, text="Source PDF:", bg=JB_BG, fg=JB_TEXT, 
                 font=("Inter", 10, "bold")).pack(anchor="w", pady=(10, 5))
        
        self.path_entry = tk.Entry(self.main_content, bg=JB_FIELD, fg="#B376FF",
                                  insertbackground=JB_TEXT, borderwidth=0,
                                  highlightbackground=JB_BORDER, highlightthickness=1,
                                  font=("JetBrains Mono", 10))
        self.path_entry.pack(fill="x", ipady=8)
        
        self.btn_browse = tk.Button(self.main_content, text="Select File...", font=("Inter", 9),
                                   bg=JB_BORDER, fg=JB_TEXT, bd=0, width=15, pady=5,
                                   command=self.select_file, cursor="hand2")
        self.btn_browse.pack(anchor="e", pady=(5, 0))

        # 2. USB Auto-Export Checkbox
        self.usb_check = tk.Checkbutton(self.main_content, text="Auto-Export to USB Flash Drive", 
                                       variable=self.usb_mode, onvalue=True, offvalue=False,
                                       bg=JB_BG, fg=JB_COMMENT, selectcolor=JB_FIELD,
                                       activebackground=JB_BG, activeforeground=JB_BLUE,
                                       highlightthickness=0, bd=0,
                                       command=self.toggle_usb_mode, font=("Inter", 10))
        self.usb_check.pack(anchor="w", pady=(20, 5))

        # 3. Output Folder Selection
        self.out_label = tk.Label(self.main_content, text="Output Directory:", bg=JB_BG, fg=JB_TEXT, 
                                 font=("Inter", 10, "bold"))
        self.out_label.pack(anchor="w", pady=(5, 5))

        self.out_entry = tk.Entry(self.main_content, bg=JB_FIELD, fg="#B376FF",
                                disabledforeground="white", disabledbackground=JB_BG,
                                highlightbackground=JB_BORDER, highlightthickness=1,
                                bd=0, font=("JetBrains Mono", 10))
        self.out_entry.pack(fill="x", ipady=8)

        self.btn_out = tk.Button(self.main_content, text="Select Folder...", font=("Inter", 9),
                                bg=JB_BORDER, fg=JB_TEXT, bd=0, width=15, pady=5,
                                command=self.select_out_dir, cursor="hand2")
        self.btn_out.pack(anchor="e", pady=(5, 0))

        # 4. Progress Bar (Created but NOT packed yet)
        self.progress = ttk.Progressbar(self.main_content, orient="horizontal", 
                                        mode="determinate", 
                                        style="JB.Horizontal.TProgressbar")

        # 5. Run Button
        self.btn_run = tk.Button(self.main_content, text="Convert PDF to .bin", font=("Inter", 10, "bold"),
                                bg=JB_BLUE, fg="white", activebackground="#2a5cc0",
                                bd=0, pady=15, command=self.run_process, cursor="hand2")
        self.btn_run.pack(fill="x", pady=(20, 10))

    def find_usb_drive(self):
        bitmask = ctypes.windll.kernel32.GetLogicalDrives()
        for letter in string.ascii_uppercase:
            if bitmask & 1:
                drive_path = f"{letter}:\\"
                if ctypes.windll.kernel32.GetDriveTypeW(drive_path) == 2:
                    return drive_path
            bitmask >>= 1
        return None

    def toggle_usb_mode(self):
        if self.usb_mode.get():
            usb_path = self.find_usb_drive()
            if usb_path:
                display_text = f"(USB Drive) {usb_path}"
                confirm = messagebox.askyesno(
                    "Confirm USB Mode",
                    f"USB Drive detected at {usb_path}\n\n"
                    "Files will be exported directly to the root of the drive.\n"
                    "Existing files will be overwritten and orphaned pages removed.\n\n"
                    "Proceed?"
                )
                if confirm:
                    self.actual_usb_path = usb_path
                    self.out_entry.config(state="normal")
                    self.out_entry.delete(0, tk.END)
                    self.out_entry.insert(0, display_text)
                    self.out_entry.config(state="disabled")
                    self.btn_out.config(state="disabled")
                else:
                    self.usb_mode.set(False)
            else:
                messagebox.showerror("USB Error", "No removable USB Flash Drive detected.")
                self.usb_mode.set(False)
        else:
            self.out_entry.config(state="normal")
            self.btn_out.config(state="normal")

    def select_file(self):
        path = filedialog.askopenfilename(filetypes=[("PDF files", "*.pdf")])
        if path:
            friendly_path = Path(path).as_posix()
            self.selected_path = friendly_path
            self.path_entry.delete(0, tk.END)
            self.path_entry.insert(0, friendly_path)

    def select_out_dir(self):
        path = filedialog.askdirectory()
        if path:
            friendly_path = Path(path).as_posix()
            self.out_entry.delete(0, tk.END)
            self.out_entry.insert(0, friendly_path)

    def run_process(self):
        if not self.selected_path:
            messagebox.showwarning("Warning", "Please select a source PDF file first.")
            return
        
        target_dir = self.actual_usb_path if self.usb_mode.get() else self.out_entry.get()
        
        if not target_dir:
            messagebox.showwarning("Warning", "Output directory is empty.")
            return

        # Show progress bar and move it above the button
        self.progress.pack(fill="x", pady=(10, 10), before=self.btn_run)
        
        self.btn_run.config(state="disabled", text="Processing...")
        threading.Thread(target=self.worker_thread, args=(target_dir,), daemon=True).start()

    def worker_thread(self, output_dir):
        try:
            exe_path = Path(self.cpp_exe).absolute()
            build_dir = exe_path.parent
            env = os.environ.copy()
            env["PATH"] = str(build_dir) + os.pathsep + env.get("PATH", "")
            
            process = subprocess.Popen(
                [str(exe_path), output_dir, self.selected_path],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                text=True, cwd=str(build_dir), env=env,
                creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0
            )

            total_pages = 0
            while True:
                line = process.stdout.readline()
                if not line: break
                if "TOTAL_PAGES:" in line:
                    total_pages = int(line.split(":")[1].strip())
                    self.progress["maximum"] = total_pages
                elif "PROGRESS_PAGE:" in line:
                    self.progress["value"] = int(line.split(":")[1].strip())
                self.master.update_idletasks()

            process.wait()
            
            if process.returncode == 0:
                # 1. Post-Process Move (USB only)
                if self.usb_mode.get():
                    bin_folder = Path(output_dir) / "binfiles"
                    preview_folder = Path(output_dir) / "previews"
                    if bin_folder.exists():
                        for bin_file in bin_folder.glob("*.bin"):
                            dest_path = Path(output_dir) / bin_file.name
                            if dest_path.exists(): os.remove(dest_path)
                            shutil.move(str(bin_file), str(dest_path))
                        shutil.rmtree(bin_folder)
                    if preview_folder.exists(): shutil.rmtree(preview_folder)

                # 2. Orphan Cleanup
                for file_name in os.listdir(output_dir):
                    match = re.match(r"music(\d+)_gxepd2\.bin", file_name)
                    if match:
                        if int(match.group(1)) > total_pages:
                            try: os.remove(os.path.join(output_dir, file_name))
                            except: pass

                messagebox.showinfo("Success", "Conversion complete!")
            else:
                stderr = process.stderr.read()
                messagebox.showerror("C++ Error", f"Code {process.returncode}:\n{stderr}")

        except Exception as e:
            messagebox.showerror("System Error", str(e))
        finally:
            self.btn_run.config(state="normal", text="Convert PDF to .bin")
            self.progress["value"] = 0
            self.progress.pack_forget() # Hide progress bar when done

if __name__ == "__main__":
    try: ctypes.windll.shcore.SetProcessDpiAwareness(1)
    except: pass
    root = tk.Tk()
    app = PDFBinConverter(master=root)
    root.mainloop()