// Prevents additional console window on Windows in release mode
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use tauri::{Manager, WindowEvent};
use std::fs;
use std::path::PathBuf;
use serde::{Serialize, Deserialize};

#[derive(Serialize, Deserialize)]
struct FileInfo {
    name: String,
    size: u64,
    modified: String,
}

#[derive(Serialize, Deserialize)]
struct FileListResult {
    success: bool,
    files: Vec<FileInfo>,
    #[serde(skip_serializing_if = "Option::is_none")]
    error: Option<String>,
}

#[derive(Serialize, Deserialize)]
struct ModuleFile {
    name: String,
    #[serde(rename = "type")]
    file_type: String,
    size: u64,
    modified: String,
}

#[derive(Serialize, Deserialize)]
struct WasmModule {
    name: String,
    files: Vec<ModuleFile>,
}

#[derive(Serialize, Deserialize)]
struct ModuleListResult {
    success: bool,
    modules: Vec<WasmModule>,
    #[serde(skip_serializing_if = "Option::is_none")]
    error: Option<String>,
}

#[derive(Serialize, Deserialize)]
struct FileContentResult {
    success: bool,
    #[serde(skip_serializing_if = "Option::is_none")]
    content: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    filename: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    error: Option<String>,
}

// File operations
#[tauri::command]
async fn open_file(path: String) -> Result<String, String> {
    fs::read_to_string(&path)
        .map_err(|e| format!("Failed to read file: {}", e))
}

#[tauri::command]
async fn save_file(path: String, content: String) -> Result<(), String> {
    fs::write(&path, content)
        .map_err(|e| format!("Failed to write file: {}", e))
}

#[tauri::command]
async fn get_file_name(path: String) -> Result<String, String> {
    let path_buf = PathBuf::from(path);
    path_buf
        .file_name()
        .and_then(|n| n.to_str())
        .map(|s| s.to_string())
        .ok_or_else(|| "Failed to get file name".to_string())
}

// Window title management
#[tauri::command]
async fn set_title(window: tauri::Window, title: String) -> Result<(), String> {
    window.set_title(&title)
        .map_err(|e| format!("Failed to set title: {}", e))
}

// File browser: Get C++ files from ~/.madola/gen_cpp
#[tauri::command]
async fn get_cpp_files() -> FileListResult {
    println!("[Rust] get_cpp_files called");
    
    let home_dir = match dirs::home_dir() {
        Some(dir) => {
            println!("[Rust] Home dir: {:?}", dir);
            dir
        },
        None => {
            println!("[Rust] ERROR: Could not determine home directory");
            return FileListResult {
                success: false,
                files: vec![],
                error: Some("Could not determine home directory".to_string()),
            };
        }
    };

    let gen_cpp_dir = home_dir.join(".madola").join("gen_cpp");
    println!("[Rust] Looking in: {:?}", gen_cpp_dir);
    
    // Create directory if it doesn't exist
    if !gen_cpp_dir.exists() {
        println!("[Rust] Directory does not exist, creating...");
        if let Err(e) = fs::create_dir_all(&gen_cpp_dir) {
            println!("[Rust] ERROR creating directory: {}", e);
            return FileListResult {
                success: false,
                files: vec![],
                error: Some(format!("Failed to create directory: {}", e)),
            };
        }
    }

    let mut files = Vec::new();

    match fs::read_dir(&gen_cpp_dir) {
        Ok(entries) => {
            for entry in entries.flatten() {
                if let Ok(file_name) = entry.file_name().into_string() {
                    if file_name.ends_with(".cpp") {
                        if let Ok(metadata) = entry.metadata() {
                            if let Ok(modified) = metadata.modified() {
                                let modified_str = format!("{:?}", modified);
                                println!("[Rust] Found C++ file: {} ({} bytes)", file_name, metadata.len());
                                files.push(FileInfo {
                                    name: file_name,
                                    size: metadata.len(),
                                    modified: modified_str,
                                });
                            }
                        }
                    }
                }
            }
        }
        Err(e) => {
            println!("[Rust] ERROR reading directory: {}", e);
            return FileListResult {
                success: false,
                files: vec![],
                error: Some(format!("Failed to read directory: {}", e)),
            };
        }
    }

    files.sort_by(|a, b| a.name.cmp(&b.name));
    println!("[Rust] Returning {} C++ files", files.len());

    FileListResult {
        success: true,
        files,
        error: None,
    }
}

// File browser: Get WASM modules from ~/.madola/trove
#[tauri::command]
async fn get_wasm_modules() -> ModuleListResult {
    println!("[Rust] get_wasm_modules called");
    
    let home_dir = match dirs::home_dir() {
        Some(dir) => {
            println!("[Rust] Home dir: {:?}", dir);
            dir
        },
        None => {
            println!("[Rust] ERROR: Could not determine home directory");
            return ModuleListResult {
                success: false,
                modules: vec![],
                error: Some("Could not determine home directory".to_string()),
            };
        }
    };

    let trove_dir = home_dir.join(".madola").join("trove");
    println!("[Rust] Looking in: {:?}", trove_dir);
    
    // Create directory if it doesn't exist
    if !trove_dir.exists() {
        println!("[Rust] Directory does not exist, creating...");
        if let Err(e) = fs::create_dir_all(&trove_dir) {
            println!("[Rust] ERROR creating directory: {}", e);
            return ModuleListResult {
                success: false,
                modules: vec![],
                error: Some(format!("Failed to create directory: {}", e)),
            };
        }
    }

    let mut modules = Vec::new();

    match fs::read_dir(&trove_dir) {
        Ok(entries) => {
            for entry in entries.flatten() {
                if let Ok(file_type) = entry.file_type() {
                    if file_type.is_dir() {
                        if let Ok(module_name) = entry.file_name().into_string() {
                            println!("[Rust] Checking module directory: {}", module_name);
                            let module_path = entry.path();
                            let mut module_files = Vec::new();

                            if let Ok(module_entries) = fs::read_dir(&module_path) {
                                for file_entry in module_entries.flatten() {
                                    if let Ok(file_name) = file_entry.file_name().into_string() {
                                        if file_name.ends_with(".wasm") || file_name.ends_with(".js") {
                                            if let Ok(metadata) = file_entry.metadata() {
                                                if let Ok(modified) = metadata.modified() {
                                                    let modified_str = format!("{:?}", modified);
                                                    let file_type = if file_name.ends_with(".wasm") {
                                                        "wasm"
                                                    } else {
                                                        "js"
                                                    };

                                                    println!("[Rust]   Found {} file: {} ({} bytes)", file_type, file_name, metadata.len());
                                                    module_files.push(ModuleFile {
                                                        name: file_name,
                                                        file_type: file_type.to_string(),
                                                        size: metadata.len(),
                                                        modified: modified_str,
                                                    });
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            if !module_files.is_empty() {
                                println!("[Rust] Added module '{}' with {} files", module_name, module_files.len());
                                modules.push(WasmModule {
                                    name: module_name,
                                    files: module_files,
                                });
                            } else {
                                println!("[Rust] Module '{}' has no .wasm or .js files, skipping", module_name);
                            }
                        }
                    }
                }
            }
        }
        Err(e) => {
            println!("[Rust] ERROR reading directory: {}", e);
            return ModuleListResult {
                success: false,
                modules: vec![],
                error: Some(format!("Failed to read directory: {}", e)),
            };
        }
    }

    modules.sort_by(|a, b| a.name.cmp(&b.name));
    println!("[Rust] Returning {} WASM modules", modules.len());

    ModuleListResult {
        success: true,
        modules,
        error: None,
    }
}

// File browser: Get C++ file content
#[tauri::command]
async fn get_cpp_file_content(filename: String) -> FileContentResult {
    let home_dir = match dirs::home_dir() {
        Some(dir) => dir,
        None => return FileContentResult {
            success: false,
            content: None,
            filename: None,
            error: Some("Could not determine home directory".to_string()),
        },
    };

    let file_path = home_dir.join(".madola").join("gen_cpp").join(&filename);
    
    if !file_path.exists() {
        return FileContentResult {
            success: false,
            content: None,
            filename: None,
            error: Some("File not found".to_string()),
        };
    }

    match fs::read_to_string(&file_path) {
        Ok(content) => FileContentResult {
            success: true,
            content: Some(content),
            filename: Some(filename),
            error: None,
        },
        Err(e) => FileContentResult {
            success: false,
            content: None,
            filename: None,
            error: Some(format!("Failed to read file: {}", e)),
        },
    }
}

fn main() {
    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![
            open_file,
            save_file,
            get_file_name,
            set_title,
            get_cpp_files,
            get_wasm_modules,
            get_cpp_file_content
        ])
        .setup(|app| {
            let window = app.get_window("main").unwrap();

            // Handle file drop events
            window.on_window_event(|event| {
                if let WindowEvent::FileDrop(tauri::FileDropEvent::Dropped(paths)) = event {
                    // Handle dropped files
                    if let Some(path) = paths.first() {
                        println!("File dropped: {:?}", path);
                        // You can emit an event to the frontend here
                    }
                }
            });

            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}