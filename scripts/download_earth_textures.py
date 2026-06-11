import urllib.request
import subprocess
import os

def download_file(url, output_path):
    print(f"Downloading {url} to {output_path}...")
    req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
    with urllib.request.urlopen(req) as response:
        with open(output_path, 'wb') as out_file:
            out_file.write(response.read())
    print("Download complete.")

def main():
    textures_dir = "textures"
    os.makedirs(textures_dir, exist_ok=True)
    
    # 1. Download Earth Night Lights
    night_url = "https://upload.wikimedia.org/wikipedia/commons/2/2f/Solarsystemscope_texture_2k_earth_nightmap.jpg"
    night_out = os.path.join(textures_dir, "earth_night.jpg")
    download_file(night_url, night_out)
    
    # 2. Download Earth Clouds
    cloud_url = "https://upload.wikimedia.org/wikipedia/commons/e/ed/Solarsystemscope_texture_2k_earth_clouds.jpg"
    cloud_out = os.path.join(textures_dir, "earth_clouds.jpg")
    download_file(cloud_url, cloud_out)
    
    # 3. Download Earth Specular Map (TIF format)
    spec_url = "https://upload.wikimedia.org/wikipedia/commons/d/db/Solarsystemscope_texture_2k_earth_specular_map.tif"
    spec_tif_out = os.path.join(textures_dir, "earth_specular_map.tif")
    download_file(spec_url, spec_tif_out)
    
    # 4. Convert TIF to JPG using macOS sips
    spec_jpg_out = os.path.join(textures_dir, "earth_specular.jpg")
    print("Converting TIF to JPG using sips...")
    try:
        subprocess.run(["sips", "-s", "format", "jpeg", spec_tif_out, "--out", spec_jpg_out], check=True)
        print("Conversion successful.")
        # Remove original TIF
        os.remove(spec_tif_out)
        print("Cleaned up temporary TIF file.")
    except Exception as e:
        print(f"Error during sips conversion: {e}")

if __name__ == "__main__":
    main()
