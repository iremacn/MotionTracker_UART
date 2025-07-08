#!/usr/bin/env python3
"""
STM32 MotionTracker GUI Setup Script
Bu script gerekli Python kütüphanelerini yükler ve GUI'yi çalıştırır
"""

import subprocess
import sys
import os

def install_requirements():
    """Gerekli kütüphaneleri yükle"""
    print("🔧 Gerekli Python kütüphaneleri yükleniyor...")
    
    try:
        # pip upgrade
        subprocess.check_call([sys.executable, "-m", "pip", "install", "--upgrade", "pip"])
        
        # Requirements yükle
        subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", "requirements.txt"])
        
        print("✅ Tüm kütüphaneler başarıyla yüklendi!")
        return True
        
    except subprocess.CalledProcessError as e:
        print(f"❌ Kütüphane yüklemede hata: {e}")
        return False

def check_python_version():
    """Python versiyonunu kontrol et"""
    version = sys.version_info
    print(f"🐍 Python versiyonu: {version.major}.{version.minor}.{version.micro}")
    
    if version.major < 3 or (version.major == 3 and version.minor < 7):
        print("⚠️  Python 3.7 veya üzeri gerekli!")
        return False
    
    print("✅ Python versiyonu uygun!")
    return True

def run_gui():
    """GUI'yi çalıştır"""
    print("🚀 MotionTracker GUI başlatılıyor...")
    try:
        os.system("python gui_interface.py")
    except Exception as e:
        print(f"❌ GUI başlatılamadı: {e}")

def main():
    print("=" * 50)
    print("🎯 STM32 MotionTracker GUI Kurulum")
    print("=" * 50)
    
    # Python version check
    if not check_python_version():
        input("Devam etmek için Enter'a basın...")
        return
    
    # Install requirements
    if not install_requirements():
        input("Devam etmek için Enter'a basın...")
        return
    
    print("\n" + "=" * 50)
    print("✅ Kurulum tamamlandı!")
    print("📋 Kullanım:")
    print("   1. STM32 cihazınızı USB ile bağlayın")
    print("   2. GUI'yi açın")
    print("   3. Doğru COM portunu seçin")
    print("   4. 'Bağlan' butonuna tıklayın")
    print("=" * 50)
    
    choice = input("\nGUI'yi şimdi başlatmak ister misiniz? (y/n): ")
    if choice.lower() in ['y', 'yes', 'evet', 'e']:
        run_gui()

if __name__ == "__main__":
    main() 